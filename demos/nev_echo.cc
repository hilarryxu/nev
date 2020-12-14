#include <stdio.h>

#include <string>

#include "nev/nev_init.h"
#include "nev/ip_endpoint.h"
#include "nev/event_loop.h"
#include "nev/tcp_server.h"

using namespace nev;

void onConnection(const TcpConnectionSharedPtr& conn) {
  if (conn->connected()) {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(const TcpConnectionSharedPtr& conn,
               Buffer* buf,
               base::TimeTicks) {
  std::string msg(buf->retrieveAllAsString());
  conn->send(msg);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: server <port>\n");
    return 1;
  } else {
    EnsureNevInit();

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    IPEndPoint listen_addr(IPAddress::IPv4Localhost(), port);

    EventLoop loop;

    TcpServer server(&loop, listen_addr, "nev_echo");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    loop.loop();

    return 0;
  }
}
