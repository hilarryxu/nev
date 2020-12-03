#include "nev/tcp_server.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "nev/acceptor.h"
#include "nev/event_loop.h"
#include "nev/event_loop_thread_pool.h"

namespace nev {

TcpServer::TcpServer(EventLoop* loop, const IPEndPoint& listen_addr)
    : loop_(loop),
      name_(listen_addr.toString()),
      acceptor_(new Acceptor(loop, listen_addr)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      started_(false),
      next_conn_id_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  LOG(DEBUG) << "TcpServer::~TcpServer [" << name_ << "] destructing";
}

void TcpServer::setThreadNum(int num_threads) {
  DCHECK(num_threads >= 0);
  thread_pool_->setThreadNum(num_threads);
}

void TcpServer::start() {
  if (!started_) {
    started_ = true;
    thread_pool_->start();
  }

  DCHECK(!acceptor_->listenning());
  if (!acceptor_->listenning()) {
    loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
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
  // 主 Reactor 记录 conn，然后下发到一个 subReactor 上去
  EventLoop* conn_loop = thread_pool_->getNextLoop();
  TcpConnectionSharedPtr conn(
      new TcpConnection(conn_loop, conn_name, sockfd, local_addr, peer_addr));
  connections_[conn_name] = conn;
  conn->setConnectionCallback(connection_cb_);
  conn->setMessageCallback(message_cb_);
  conn->setWriteCompleteCallback(write_complete_cb_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, _1));  // FIXME(xcc): unsafe
  // 提交到连接对应的 loop 上执行
  conn_loop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionSharedPtr& conn) {
  // FIXME(xcc): unsafe
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionSharedPtr& conn) {
  loop_->assertInLoopThread();
  LOG(INFO) << "TcpServer::removeConnectionInLoop [" << name_
            << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  (void)n;
  DCHECK(n == 1);
  // 相应的断开连接操作也要提交到连接对应 loop 上执行
  EventLoop* conn_loop = conn->loop();
  // 从 connections_ 移除后调用 conn->connectDestroyed
  // 延迟到 pending_functors_ 中执行连接销毁
  // 使得原先的 Channel::handleEvent 能正常执行完毕
  // 所以这里用的是 queueInLoop，而不是 runInLoop。
  conn_loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace nev
