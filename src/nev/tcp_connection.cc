#include "nev/tcp_connection.h"

#include "base/logging.h"

#include "nev/channel.h"
#include "nev/event_loop.h"
#include "nev/socket.h"
#include "nev/sockets_ops.h"
#include "nev/macros.h"

namespace nev {

namespace {

bool isFaultError(int saved_errno) {
  if (saved_errno == WSAECONNRESET)
    return true;
  return false;
}

void handleForceCloseWithDelay(const std::weak_ptr<TcpConnection>& wp_conn) {
  std::shared_ptr<TcpConnection> sp_conn(wp_conn.lock());
  if (sp_conn) {
    sp_conn->forceClose();
  }
}

}  // namespace

TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& name,
                             SocketDescriptor sockfd,
                             const IPEndPoint& local_addr,
                             const IPEndPoint& peer_addr)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd, socket_->fd())),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024),
      context_(nullptr) {
  LOG(DEBUG) << "TcpConnection::ctor[" << name_ << "] at " << this
             << " sockfd = " << sockfd;
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG(DEBUG) << "TcpConnection::dtor[" << name_ << "] at " << this
             << " sockfd = " << channel_->sockfd()
             << " state = " << stateToString();
  DCHECK(state_ == kDisconnected);

  if (context_ && context_cleanup_cb_) {
    context_cleanup_cb_(context_);
  }
}

void TcpConnection::send(const void* data, size_t len) {
  send(base::StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const base::StringPiece& message) {
  // 连接状态下才能发送数据
  // 也就是说 shutdown 之后状态改变不能再写数据了
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      // loop 线程中直接发送
      sendInLoop(message);
    } else {
      // 其他线程中调用就投递一个 functor
      void (TcpConnection::*fp)(const base::StringPiece& message) =
          &TcpConnection::sendInLoop;
      // NOTE: 这里复制了一份数据到绑定的 functor 参数中
      // 有一些性能开销但保证的发送过程中数据的访问安全性
      loop_->runInLoop(std::bind(fp, this, message.as_string()));
    }
  }
}

void TcpConnection::send(Buffer* buf) {
  // 连接状态下才能发送数据
  // 也就是说 shutdown 之后状态改变不能再写数据了
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      // loop 线程中直接发送
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      // 其他线程中调用就投递一个 functor
      void (TcpConnection::*fp)(const base::StringPiece& message) =
          &TcpConnection::sendInLoop;
      // FIXME(xcc): 性能优化？
      loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
    }
  }
}

void TcpConnection::shutdown() {
  // FIXME(xcc): use compare and swap
  if (state_ == kConnected) {
    // 设置状态为正在关闭连接
    setState(kDisconnecting);
    // FIXME(xcc): shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::forceClose() {
  // FIXME(xcc): use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnecting);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnecting);
    std::weak_ptr<TcpConnection> wp_conn(shared_from_this());
    loop_->runAfter(seconds, std::bind(&handleForceCloseWithDelay, wp_conn));
  }
}

void TcpConnection::setTcpNoDelay(bool on) {
  socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  DCHECK(state_ == kConnecting);
  setState(kConnected);
  // NOTE: channel_ 中关联一下 TcpConnection，类似弱回调
  // 确保 TcpConnection 存活时事件触发调用 TcpConnection::handleRead
  // 等函数的安全性。目前感觉可能性较小，除非 TcpConnection::connectDestroyed
  // 后又往队列里加了 functor
  channel_->tie(shared_from_this());
  channel_->enableReading();

  // 连接建立回调
  connection_cb_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  // 通常 handleClose 时已经将状态置为 kDisconnected
  if (state_ == kConnected) {
    // NOTE: 该分支什么情况下触发？
    setState(kDisconnected);
    // 停止关注任何事件
    channel_->disableAll();
    // 连接断开回调
    connection_cb_(shared_from_this());
  }

  // 从反应器中移除 channel_
  channel_->remove();
  // NOTE: 执行完该函数后就开始销毁连接及相关资源
}

void TcpConnection::handleRead(base::TimeTicks receive_time) {
  int saved_errno = 0;
  ssize_t n = input_buffer_.readFd(channel_->sockfd(), &saved_errno);
  if (n > 0) {
    message_cb_(shared_from_this(), &input_buffer_, receive_time);
  } else if (n == 0) {
    handleClose();
  } else {
    LOG(ERROR) << "TcpConnection::handleRead saved_errno = " << saved_errno;
    handleError(saved_errno);
    if (isFaultError(saved_errno))
      handleClose();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = sockets::Write(channel_->sockfd(), output_buffer_.peek(),
                               output_buffer_.readableBytes());
    if (n > 0) {
      // 消费输出缓冲区 n 个字节
      output_buffer_.retrieve(n);
      if (output_buffer_.readableBytes() == 0) {
        // 全部发送完了就暂时停止关注可写事件
        channel_->disableWriting();
        if (write_complete_cb_) {
          loop_->queueInLoop(std::bind(write_complete_cb_, shared_from_this()));
        }
        // 写完后判断是否需要 shutdown 关闭写端
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      } else {
        LOG(DEBUG) << "I am going to write more data";
      }
    } else {
      LOG(ERROR) << "TcpConnection::handleWrite";
    }
  } else {
    LOG(DEBUG) << "Connection sockfd = " << channel_->sockfd()
               << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG(DEBUG) << "TcpConnection::handleClose sockfd = " << channel_->sockfd()
             << " state = " << stateToString();
  DCHECK(state_ == kConnected || state_ == kDisconnecting);
  // 连接已经不能继续使用了，可以标记为 kDisconnected，
  // 不必等到 TcpConnection::connectDestroyed 再标记。
  setState(kDisconnected);
  // 停止关注任何事件
  channel_->disableAll();
  TcpConnectionSharedPtr guard_conn(shared_from_this());
  // 调用断开连接回调
  connection_cb_(guard_conn);
  // 最后调用 close_cb_
  // 其实是调用 TcpServer::removeConnection，从 TcpServer 中移除后
  // 再调用 TcpConnection::connectDestroyed
  close_cb_(guard_conn);
}

void TcpConnection::handleError(int saved_errno) {
  int err = sockets::GetSocketError(channel_->sockfd(), saved_errno);
  LOG(ERROR) << "TcpConnection::handleError [" << name_
             << "] - SO_ERROR = " << err;
}

void TcpConnection::sendInLoop(const base::StringPiece& message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;

  // FIXME(xcc): 严重套接字错误处理
  if (state_ == kDisconnected) {
    LOG(WARNING) << "disconnected, give up writing";
    return;
  }

  // 没有在写数据并且输出缓冲区为空，那么可以直接发送数据。
  if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
    nwrote = sockets::Write(channel_->sockfd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (implicit_cast<size_t>(nwrote) < len) {
        // 没写完得下面会追加到输出缓冲区中去，等待下次发送。
        LOG(DEBUG) << "I am going to write more data";
      } else if (write_complete_cb_) {
        loop_->queueInLoop(std::bind(write_complete_cb_, shared_from_this()));
      }
    } else {
      // 发送数据出错了
      nwrote = 0;
      int saved_errno = WSAGetLastError();
      if (saved_errno != WSAEWOULDBLOCK) {
        LOG(ERROR) << "TcpConnection::sendInLoop";
      }
    }
  }

  DCHECK(remaining <= len);
  // 正在写的话就不能调用 sockets::Write 了，会乱序，只能先放到输出缓冲区。
  if (remaining > 0) {
    // 判断高水位，且仅在首次超水位时调用一次。
    size_t old_len = output_buffer_.readableBytes();
    if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_ &&
        high_water_mark_cb_) {
      loop_->queueInLoop(std::bind(high_water_mark_cb_, shared_from_this(),
                                   old_len + remaining));
    }

    // 把没写完的追加到输出缓冲区
    output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);
    // 没写完就继续关注可写事件
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  // 不再写时关闭套接字发送数据端
  // 还在写时不执行任何操作，等待写完后会自动调用该函数
  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

const char* TcpConnection::stateToString() const {
  switch (state_) {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

}  // namespace nev
