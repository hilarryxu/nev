#include "nev/connector.h"

#include "build/build_config.h"
#include "base/logging.h"

#include "nev/ip_endpoint.h"
#include "nev/sockets_ops.h"
#include "nev/channel.h"
#include "nev/event_loop.h"

#if defined(OS_WIN)
#include <io.h>
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) _open_osfhandle(handle, 0)
#else
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) handle
#endif

namespace nev {

Connector::Connector(EventLoop* loop, const IPEndPoint& server_addr)
    : loop_(loop), server_addr_(server_addr), state_(kDisconnected) {
  LOG(DEBUG) << "Connector::ctor at " << this;
}

Connector::~Connector() {
  LOG(DEBUG) << "Connector::dtor at " << this;
}

void Connector::start() {
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::stop() {
  connect_ = false;
}

void Connector::startInLoop() {
  loop_->assertInLoopThread();
  DCHECK(state_ == kDisconnected);
  if (connect_) {
    connect();
  } else {
    LOG(DEBUG) << "do not connect";
  }
}

void Connector::connect() {
  SocketDescriptor sockfd = sockets::CreateNonblockingOrDie();
  int rv = sockets::Connect(sockfd, server_addr_);
  int saved_errno = 0;
  if (rv == SOCKET_ERROR)
    saved_errno = WSAGetLastError();

  switch (saved_errno) {
    case 0:
    case WSAEWOULDBLOCK:
    case WSAEALREADY:
    case WSAEISCONN:
      // 非阻塞模式下不一定能立马连上
      // 需要关注套接字是否可写来判断是否连上了
      // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
      connecting(sockfd);
      break;
    default:
      LOG(ERROR) << "Unexpected error in Connector::connect " << saved_errno;
      sockets::Close(sockfd);
      break;
  }
}

void Connector::connecting(SocketDescriptor sockfd) {
  setState(kConnecting);
  DCHECK(!channel_);
  // FIXME(xcc): windows 环境下这个打开的文件描述符需要调用 _close 关闭吗？
  channel_.reset(new Channel(loop_, sockfd, EV_SOCKETDESCRIPTOR_TO_FD(sockfd)));
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));

  // 关注可写事件
  channel_->enableWriting();
}

// 销毁为了判断连接是否建立的 channel
SocketDescriptor Connector::removeAndResetChannel() {
  channel_->disableAll();
  loop_->removeChannel(channel_.get());
  SocketDescriptor sockfd = channel_->sockfd();
  // 还在 Channel::handleEvent 函数中，所以不能直接 channel_.reset()。
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
  return sockfd;
}

void Connector::resetChannel() {
  channel_.reset();
}

void Connector::handleWrite() {
  LOG(DEBUG) << "Connector::handleWrite " << state_;

  if (state_ == kConnecting) {
    // 销毁 channel 并检测套接字错误
    // 没有错误就表示连接建立成功了
    SocketDescriptor sockfd = removeAndResetChannel();
    int err = sockets::GetSocketError(sockfd);
    if (err != 0) {
      LOG(WARNING) << "Connector::handleWrite - SO_ERROR = " << err;
      sockets::Close(sockfd);
    } else {
      setState(kConnected);
      if (connect_) {
        // 连接建立成功，调用用户注册的回调函数。
        // 由调用方再去生成 TcpConnection
        new_connection_cb_(sockfd);
      } else {
        // 之前调用过 Connector::stop()，那么就关闭套接字。
        sockets::Close(sockfd);
      }
    }
  } else {
    // 应该不会进入这个分支
    DCHECK(state_ == kDisconnected);
  }
}

}  // namespace nev
