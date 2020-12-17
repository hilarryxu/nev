#include "examples/simple/discard/discard_server.h"

using namespace nev;

DiscardServer::DiscardServer(EventLoop* loop, const IPEndPoint& listen_addr)
    : server_(loop, listen_addr, "DiscardServer") {
  server_.setConnectionCallback(
      std::bind(&DiscardServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&DiscardServer::onMessage, this, _1, _2, _3));
}

void DiscardServer::start() {
  server_.start();
}

void DiscardServer::onConnection(const TcpConnectionSharedPtr& conn) {
  LOG(INFO) << "DiscardServer - " << conn->peer_addr().toString() << " -> "
            << conn->local_addr().toString() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

void DiscardServer::onMessage(const TcpConnectionSharedPtr& conn,
                              Buffer* buf,
                              base::TimeTicks receive_time) {
  std::string msg(buf->retrieveAllAsString());
  LOG(INFO) << conn->name() << " discards " << msg.size()
            << " bytes received at " << receive_time;
}
