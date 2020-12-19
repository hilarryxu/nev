#include "contrib/thrift/thrift_server.h"

#include "base/logging.h"

#include "nev/event_loop.h"

namespace nev {

ThriftServer::ThriftServer(const std::shared_ptr<TProcessor>& processor,
                           EventLoop* loop,
                           const IPEndPoint& listen_addr,
                           const std::string& name)
    : TServer(processor), server_(loop, listen_addr, name) {
  server_.setConnectionCallback(
      std::bind(&ThriftServer::onConnection, this, _1));
}

ThriftServer::~ThriftServer() = default;

void ThriftServer::serve() {
  server_.start();
}

void ThriftServer::stop() {
  server_.loop()->runAfter(3.0, std::bind(&EventLoop::quit, server_.loop()));
}

void ThriftServer::setThreadNum(int num_threads) {
  server_.setThreadNum(num_threads);
}

void ThriftServer::onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    ThriftConnectionSharedPtr sp_thrift_conn(new ThriftConnection(this, conn));
    {
      base::AutoLock lock(connections_lock_);
      DCHECK(connections_.find(conn->name()) == connections_.end());
      connections_[conn->name()] = sp_thrift_conn;
    }
  } else {
    {
      base::AutoLock lock(connections_lock_);
      DCHECK(connections_.find(conn->name()) != connections_.end());
      connections_.erase(conn->name());
    }
  }
}

}  // namespace nev
