#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_server.h"

using namespace nev;

std::string message1;
std::string message2;

void onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(), conn->peer_addr().toString().c_str());
    conn->send(message1);
    conn->send(message2);
    conn->shutdown();
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

int main(int argc, char* argv[]) {
  EnsureNevInit();

  int len1 = 100;
  int len2 = 200;

  if (argc > 2) {
    len1 = atoi(argv[1]);
    len2 = atoi(argv[2]);
  }

  message1.resize(len1);
  message2.resize(len2);
  std::fill(message1.begin(), message1.end(), 'A');
  std::fill(message2.begin(), message2.end(), 'B');

  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  TcpServer server(&loop, listen_addr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.start();

  loop.loop();
}
