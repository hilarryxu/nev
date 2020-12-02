#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_server.h"

using namespace nev;

void onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peer_addr().toString().c_str());
  } else {
    printf("onConnection(): connection [%s] is down\n", conn->name().c_str());
  }
}

void onMessage(const TcpConnectionSharedPtr& conn,
               const char* data,
               ssize_t len) {
  printf("onMessage(): received %zd bytes from connection [%s]\n", len,
         conn->name().c_str());
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  TcpServer server(&loop, listen_addr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.start();

  loop.loop();
}
