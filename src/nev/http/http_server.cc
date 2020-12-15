#include "nev/http/http_server.h"

#include "base/logging.h"

#include "nev/http/http_context.h"
#include "nev/http/http_request.h"
#include "nev/http/http_response.h"

namespace nev {

namespace {

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp) {
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

void freeContext(void* ptr) {
  HttpContext* context = static_cast<HttpContext*>(ptr);
  delete context;
}

}  // namespace

HttpServer::HttpServer(EventLoop* loop,
                       const IPEndPoint& listen_addr,
                       const std::string& name,
                       bool reuse_port)
    : server_(loop, listen_addr, name, reuse_port),
      http_cb_(defaultHttpCallback) {
  server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start() {
  LOG(DEBUG) << "HttpServer starts listening";
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    conn->setContext(new HttpContext(), freeContext);
  }
}

void HttpServer::onMessage(const TcpConnectionSharedPtr& conn,
                           Buffer* buf,
                           base::TimeTicks receive_time) {
  HttpContext* context = static_cast<HttpContext*>(conn->getContext());

  if (!context->parseRequest(buf)) {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->gotAll()) {
    onRequest(conn, context->request());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionSharedPtr& conn,
                           const HttpRequest& req) {
  const std::string& connection = req.getHeader("Connection");
  bool flag_close =
      connection == "close" ||
      (req.version() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse resp(flag_close);

  http_cb_(req, &resp);

  Buffer buf;
  resp.appendToBuffer(&buf);
  conn->send(&buf);
  if (resp.closeConnection()) {
    conn->shutdown();
  }
}

}  // namespace nev
