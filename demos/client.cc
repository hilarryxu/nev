#include "nev/sockets_ops.h"
#include "nev/socket.h"
#include "nev/ip_endpoint.h"

using namespace nev;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: client <port>\n");
    return 1;
  }

  sockets::EnsureSocketInit();

  uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
  IPEndPoint server_addr(IPAddress::IPv4Localhost(), port);

  Socket client_sock(sockets::CreateOrDie());
  sockets::Connect(client_sock.fd(), server_addr);

  char buf[1024] = {0};
  sockets::Read(client_sock.fd(), buf, sizeof buf);
  printf("Message form server: %s\n", buf);

  return 0;
}
