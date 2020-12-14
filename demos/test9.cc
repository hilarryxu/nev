#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_server.h"

using namespace nev;

std::string message;

void onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peer_addr().toString().c_str());
    conn->send(message);
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void onMessage(const TcpConnectionSharedPtr& conn,
               Buffer* buf,
               base::TimeTicks receive_time) {
  LOG(DEBUG) << "onMessage(): received " << buf->readableBytes()
             << " bytes from connection [" << conn->name() << "] at "
             << receive_time;

  buf->retrieveAllAsString();
}

void onWriteComplete(const TcpConnectionSharedPtr& conn) {
  conn->send(message);
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  std::string line;
  for (int i = 33; i < 127; ++i) {
    line.push_back(char(i));
  }
  line += line;

  for (size_t i = 0; i < 127 - 33; ++i) {
    message += line.substr(i, 72) + '\n';
  }

  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  TcpServer server(&loop, listen_addr, "test9");
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.setWriteCompleteCallback(onWriteComplete);
  server.start();

  loop.loop();
}
