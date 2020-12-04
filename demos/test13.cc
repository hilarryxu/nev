#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/tcp_client.h"

using namespace nev;

std::string message = "Hello\n";

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

  printf("onMessage(): [%s]\n", buf->retrieveAllAsString().c_str());
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  IPEndPoint server_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  TcpClient client(&loop, server_addr);
  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.connect();

  loop.loop();
}
