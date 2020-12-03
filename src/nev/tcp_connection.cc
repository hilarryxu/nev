#include "nev/tcp_connection.h"

#include "base/logging.h"

#include "nev/channel.h"
#include "nev/event_loop.h"
#include "nev/socket.h"
#include "nev/sockets_ops.h"

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
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
}

TcpConnection::~TcpConnection() {
  LOG(DEBUG) << "TcpConnection::dtor[" << name_ << "] at " << this
             << " sockfd = " << channel_->sockfd();
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

void TcpConnection::handleRead() {
  char buf[65536];
  ssize_t n = sockets::Read(channel_->sockfd(), buf, sizeof buf);
  if (n > 0) {
    message_cb_(shared_from_this(), buf, n);
  } else if (n == 0) {
    handleClose();
  } else {
    LOG(ERROR) << "TcpConnection::handleRead";
    handleError();
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

}  // namespace nev
