#pragma once

#include <map>

#include <thrift/server/TServer.h>

#include "base/synchronization/lock.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_server.h"

#include "contrib/thrift/thrift_connection.h"

using apache::thrift::TProcessor;
using apache::thrift::TProcessorFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::server::TServer;
using apache::thrift::transport::TTransportFactory;

namespace nev {

class EventLoop;
class EventLoopThreadPool;

class NEV_EXPORT ThriftServer : NonCopyable, public TServer {
 public:
  ThriftServer(const std::shared_ptr<TProcessor>& processor,
               EventLoop* loop,
               const IPEndPoint& listen_addr,
               const std::string& name);
  ~ThriftServer() override;

  void serve() override;

  void stop() override;

  bool hasWorkerThreadPool() const { return num_worker_threads_ > 0; }
  std::shared_ptr<EventLoopThreadPool> workerThreadPool() {
    return worker_thread_pool_;
  }

  void setThreadNum(int num_threads);
  void setWokerThreadNum(int num_worker_threads);

 private:
  friend class ThriftConnection;

  void onConnection(const TcpConnectionSharedPtr& conn);

  using ConnectionMap = std::map<std::string, ThriftConnectionSharedPtr>;

  TcpServer server_;
  int num_worker_threads_;
  std::shared_ptr<EventLoopThreadPool> worker_thread_pool_;
  base::Lock connections_lock_;
  ConnectionMap connections_;  // Guarded by connections_lock_
};

}  // namespace nev
