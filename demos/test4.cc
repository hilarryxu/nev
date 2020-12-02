#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"
#include "nev/acceptor.h"
#include "nev/sockets_ops.h"

using namespace nev;

void newConnection(SocketDescriptor sockfd, const IPEndPoint& peer_addr) {
  printf("newConnection(): accepted a new connection from %s\n",
         peer_addr.toString().c_str());
  sockets::Write(sockfd, "How are you?\n", 13);
  sockets::Close(sockfd);
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9981);
  EventLoop loop;

  Acceptor acceptor(&loop, listen_addr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  loop.loop();
}
