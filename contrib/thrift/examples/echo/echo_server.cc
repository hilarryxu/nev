#include <stdio.h>

#include "nev/socket_descriptor.h"
#include "base/logging.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"
#include "contrib/thrift/thrift_server.h"

#include "Echo.h"

using namespace nev;

using namespace echo;

class EchoHandler : virtual public EchoIf {
 public:
  EchoHandler() {}

  void echo(std::string& _return, const std::string& arg) {
    LOG(INFO) << "EchoHandler::echo:" << arg;
    _return = arg;
  }
};

int main(int argc, char* argv[]) {
  EnsureNevInit();

  EventLoop loop;
  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), 9090);

  std::shared_ptr<EchoHandler> handler(new EchoHandler());
  std::shared_ptr<TProcessor> processor(new EchoProcessor(handler));

  ThriftServer server(processor, &loop, listen_addr, "EchoServer");
  server.setThreadNum(4);
  server.serve();
  loop.loop();

  return 0;
}
