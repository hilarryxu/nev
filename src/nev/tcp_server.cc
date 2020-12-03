#include "nev/tcp_server.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "nev/acceptor.h"
#include "nev/event_loop.h"

namespace nev {

TcpServer::TcpServer(EventLoop* loop, const IPEndPoint& listen_addr)
    : loop_(loop),
      name_(listen_addr.toString()),
      acceptor_(new Acceptor(loop, listen_addr)),
      started_(false),
      next_conn_id_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  LOG(DEBUG) << "TcpServer::~TcpServer [" << name_ << "] destructing";
}

void TcpServer::start() {
  if (!started_) {
    started_ = true;
  }

  DCHECK(!acceptor_->listenning());
  if (!acceptor_->listenning()) {
    // FIXME(xcc): 放到 loop 中执行
    acceptor_->listen();
  }
}

void TcpServer::newConnection(SocketDescriptor sockfd,
                              const IPEndPoint& peer_addr) {
  loop_->assertInLoopThread();

  std::string conn_name = name_ + base::StringPrintf("#%d", next_conn_id_);
  ++next_conn_id_;
  LOG(INFO) << "TcpServer::newConnection [" << name_ << "] - new connection ["
            << conn_name << "] from " << peer_addr.toString();

  // FIXME(xcc): GetLocalAddr
  // IPEndPoint localAddr(sockets::GetLocalAddr(sockfd));
  IPEndPoint local_addr;
  TcpConnectionSharedPtr conn(
      new TcpConnection(loop_, conn_name, sockfd, local_addr, peer_addr));
  connections_[conn_name] = conn;
  conn->setConnectionCallback(connection_cb_);
  conn->setMessageCallback(message_cb_);
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
  conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionSharedPtr& conn) {
  loop_->assertInLoopThread();
  LOG(INFO) << "TcpServer::removeConnection [" << name_ << "] - connection "
            << conn->name();
  size_t n = connections_.erase(conn->name());
  (void)n;
  DCHECK(n == 1);
  // 从 connections_ 移除后调用 conn->connectDestroyed
  conn->connectDestroyed();
  // NOTE: 这里就会调用 TcpConnection 的析构函数
}

}  // namespace nev
