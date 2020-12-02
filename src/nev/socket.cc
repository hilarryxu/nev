#include "nev/socket.h"

#include "base/logging.h"

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

void Socket::shutdownWrite() {
  sockets::ShutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
#else
  if (on) {
    LOG(ERROR) << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof(optval));
}

}  // namespace nev
