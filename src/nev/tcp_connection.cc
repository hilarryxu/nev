#include "nev/tcp_connection.h"

#include "base/logging.h"

#include "nev/channel.h"
#include "nev/event_loop.h"
#include "nev/socket.h"
#include "nev/sockets_ops.h"
#include "nev/macros.h"

namespace nev {

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
      peer_addr_(peer_addr) {
  LOG(DEBUG) << "TcpConnection::ctor[" << name_ << "] at " << this
             << " sockfd = " << sockfd;
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
}

TcpConnection::~TcpConnection() {
  LOG(DEBUG) << "TcpConnection::dtor[" << name_ << "] at " << this
             << " sockfd = " << channel_->sockfd();
}

void TcpConnection::send(const std::string& message) {
  // 连接状态下才能发送数据
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      // loop 线程中直接发送
      sendInLoop(message);
    } else {
      // 其他线程中调用就投递一个 functor
      loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
    }
  }
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  DCHECK(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();
  // 连接建立回调
  connection_cb_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  DCHECK(state_ == kConnected);
  setState(kDisconnected);
  // 停止关注任何事件
  channel_->disableAll();
  // 连接断开回调
  connection_cb_(shared_from_this());
  // 从反应器中移除 channel_
  loop_->removeChannel(channel_.get());
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
    handleError();
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
        // if (state_ == kDisconnecting) {
        //   shutdownInLoop();
        // }
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
             << " state = " << state_;
  DCHECK(state_ == kConnected);
  // 停止关注任何事件
  channel_->disableAll();
  // 最后调用 close_cb_
  close_cb_(shared_from_this());
}

void TcpConnection::handleError() {
  int err = sockets::GetSocketError(channel_->sockfd());
  LOG(ERROR) << "TcpConnection::handleError [" << name_
             << "] - SO_ERROR = " << err;
}

void TcpConnection::sendInLoop(const std::string& message) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;

  // 没有在写数据并且输出缓冲区为空，那么可以直接发送数据。
  if (!channel_->isWriting() && output_buffer_.readableBytes() == 0) {
    nwrote = sockets::Write(channel_->sockfd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (implicit_cast<size_t>(nwrote) < message.size()) {
        // 没写完得下面会追加到输出缓冲区中去，等待下次发送。
        LOG(DEBUG) << "I am going to write more data";
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

  DCHECK(nwrote >= 0);
  // 正在写的话就不能调用 sockets::Write 了，会乱序，只能先放到输出缓冲区。
  if (implicit_cast<size_t>(nwrote) < message.size()) {
    // 把没写完的追加到输出缓冲区
    output_buffer_.append(message.data() + nwrote, message.size() - nwrote);
    // 没写完就继续关注可写事件
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

}  // namespace nev
