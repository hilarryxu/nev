#include "nev/sockets_ops.h"
#include "nev/socket.h"
#include "nev/ip_endpoint.h"

using namespace nev;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: server <port>\n");
    return 1;
  }

  sockets::EnsureSocketInit();

  uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
  IPEndPoint listen_addr(IPAddress::IPv4Localhost(), port);

  Socket server_sock(sockets::CreateOrDie());
  server_sock.bindAddress(listen_addr);
  server_sock.listen();

  IPEndPoint peeraddr;
  Socket client_sock(server_sock.accept(&peeraddr, false));

  char buf[] = "Hello world";
  sockets::Write(client_sock.fd(), buf, sizeof buf);

  return 0;
}
