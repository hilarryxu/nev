#include "nev/tcp_server.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "nev/acceptor.h"
#include "nev/event_loop.h"
#include "nev/event_loop_thread_pool.h"

namespace nev {

namespace {

void defaultConnectionCallback(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peer_addr().toString().c_str());
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void defaultMessageCallback(const TcpConnectionSharedPtr& conn,
                            Buffer* buf,
                            base::TimeTicks receive_time) {
  buf->retrieveAll();
}

}  // namespace

TcpServer::TcpServer(EventLoop* loop,
                     const IPEndPoint& listen_addr,
                     const std::string& name,
                     bool reuse_port)
    : loop_(loop),
      name_(name),
      acceptor_(new Acceptor(loop, listen_addr, reuse_port)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      connection_cb_(defaultConnectionCallback),
      message_cb_(defaultMessageCallback),
      next_conn_id_(1) {
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  LOG(DEBUG) << "TcpServer::~TcpServer [" << name_ << "] destructing";

  // 销毁 connections
  for (auto& item : connections_) {
    TcpConnectionSharedPtr conn(item.second);
    item.second.reset();
    conn->loop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int num_threads) {
  DCHECK(num_threads >= 0);
  thread_pool_->setThreadNum(num_threads);
}

void TcpServer::start() {
  if (base::subtle::NoBarrier_CompareAndSwap(&started_, 0, 1) == 0) {
    thread_pool_->start(thread_init_cb_);

    DCHECK(!acceptor_->listening());
    if (!acceptor_->listening()) {
      loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
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
