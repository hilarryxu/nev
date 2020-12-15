#pragma once

#include <functional>
#include <string>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/tcp_server.h"

namespace nev {

class HttpRequest;
class HttpResponse;

// 一个同步的简易 HttpServer
class NEV_EXPORT HttpServer : NonCopyable {
 public:
  using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

  HttpServer(EventLoop* loop,
             const IPEndPoint& listen_addr,
             const std::string& name,
             bool reuse_port = false);

  EventLoop* loop() const { return server_.loop(); }

  void setHttpCallback(HttpCallback cb) { http_cb_ = std::move(cb); }

  void setThreadNum(int num_threads) { server_.setThreadNum(num_threads); }

  void start();

 private:
  void onConnection(const TcpConnectionSharedPtr& conn);
  void onMessage(const TcpConnectionSharedPtr& conn,
                 Buffer* buf,
                 base::TimeTicks receive_time);
  void onRequest(const TcpConnectionSharedPtr&, const HttpRequest&);

  TcpServer server_;
  HttpCallback http_cb_;
};

}  // namespace nev
