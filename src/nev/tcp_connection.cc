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
      channel_(new Channel(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr) {
  LOG(DEBUG) << "TcpConnection::ctor[" << name_ << "] at " << this
             << " fd=" << sockfd;
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
}

TcpConnection::~TcpConnection() {
  LOG(DEBUG) << "TcpConnection::dtor[" << name_ << "] at " << this
             << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  DCHECK(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();

  connection_cb_(shared_from_this());
}

void TcpConnection::handleRead() {
  char buf[65536];
  ssize_t n = sockets::Read(channel_->fd(), buf, sizeof buf);
  message_cb_(shared_from_this(), buf, n);
  // FIXME: close connection if n == 0
}

}  // namespace nev
