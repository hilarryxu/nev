#include "nev/socket.h"

#include "nev/sockets_ops.h"
#include "nev/ip_endpoint.h"

namespace nev {

Socket::~Socket() {
  sockets::Close(sockfd_);
}

void Socket::bindAddress(const IPEndPoint& addr) {
  sockets::BindOrDie(sockfd_, addr);
}

void Socket::listen() {
  sockets::ListenOrDie(sockfd_);
}

SocketDescriptor Socket::accept(IPEndPoint* peeraddr, bool nonblocking) {
  return sockets::Accept(sockfd_, peeraddr, nonblocking);
}

}  // namespace nev
