#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/connector.h"

using namespace nev;

EventLoop* g_loop;

void connectCallback(SocketDescriptor sockfd) {
  printf("connected.\n");
  g_loop->quit();
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  EventLoop loop;
  g_loop = &loop;

  IPEndPoint server_addr(IPAddress::IPv4Localhost(), 9981);

  ConnectorSharedPtr connector(new Connector(&loop, server_addr));
  connector->setNewConnectionCallback(connectCallback);
  connector->start();

  loop.loop();
}
