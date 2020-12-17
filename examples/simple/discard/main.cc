#include "examples/simple/discard/discard_server.h"

#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "nev/ip_endpoint.h"

using namespace nev;

int main(int argc, char* argv[]) {
  EnsureNevInit();

  EventLoop loop;
  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 2009);
  DiscardServer server(&loop, listen_addr);
  server.start();
  loop.loop();
}
