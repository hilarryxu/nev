#include "nev/sockets_ops.h"

#include "build/build_config.h"
#include "base/logging.h"
#include "nev/sys_addrinfo.h"
#include "nev/ip_endpoint.h"
#include "nev/sockaddr_storage.h"

#if defined(OS_WIN)
#include "nev/winsock_init.h"
#endif

namespace nev {

namespace {

void SetNonBlockAndCloseOnExec(SocketDescriptor sockfd) {
  unsigned long on = 1;
  ::ioctlsocket(sockfd, FIONBIO, &on);
}

}  // namespace

void sockets::EnsureSocketInit() {
#if defined(OS_WIN)
  EnsureWinsockInit();
#endif  // OS_WIN
}

SocketDescriptor sockets::CreateOrDie() {
  SocketDescriptor sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(FATAL) << "sockets::CreateOrDie";
  }

  return sockfd;
}

SocketDescriptor sockets::CreateNonblockingOrDie() {
  SocketDescriptor sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(FATAL) << "sockets::CreateNonblockingOrDie";
  }

  SetNonBlockAndCloseOnExec(sockfd);
  return sockfd;
}

int sockets::Connect(SocketDescriptor sockfd, const IPEndPoint& address) {
  SockaddrStorage storage;
  if (!address.toSockAddr(storage.addr, &storage.addr_len)) {
    LOG(FATAL) << "address invalid";
  }

  return ::connect(sockfd, storage.addr, storage.addr_len);
}

void sockets::BindOrDie(SocketDescriptor sockfd, const IPEndPoint& address) {
  DCHECK_NE(sockfd, kInvalidSocket);

  SockaddrStorage storage;
  if (!address.toSockAddr(storage.addr, &storage.addr_len)) {
    LOG(FATAL) << "address invalid";
  }

  int result = ::bind(sockfd, storage.addr, storage.addr_len);
  if (result < 0) {
    LOG(FATAL) << "bind() returned an error";
  }
}

void sockets::ListenOrDie(SocketDescriptor sockfd) {
  int result = ::listen(sockfd, SOMAXCONN);
  if (result < 0) {
    LOG(FATAL) << "listen() returned an error";
  }
}

SocketDescriptor sockets::Accept(SocketDescriptor sockfd,
                                 IPEndPoint* address,
                                 bool nonblocking) {
  DCHECK(sockfd);
  DCHECK(address);

  SockaddrStorage storage;
  SocketDescriptor new_socket =
      ::accept(sockfd, storage.addr, &storage.addr_len);
  if (new_socket < 0) {
    int saved_errno = WSAGetLastError();
    if (saved_errno != WSAEWOULDBLOCK)
      LOG(FATAL) << "unexpected error of accept, error " << saved_errno;
  } else {
    IPEndPoint ip_end_point;
    if (!ip_end_point.fromSockAddr(storage.addr, storage.addr_len)) {
      NOTREACHED();
      if (::closesocket(new_socket) < 0)
        PLOG(ERROR) << "closesocket";
    }
    *address = ip_end_point;

    if (nonblocking)
      SetNonBlockAndCloseOnExec(new_socket);
  }
  return new_socket;
}

ssize_t sockets::Read(SocketDescriptor sockfd, void* buf, size_t count) {
  return ::recv(sockfd, (char*)buf, count, 0);
}

ssize_t sockets::Write(SocketDescriptor sockfd, const void* buf, size_t count) {
  return ::send(sockfd, (const char*)buf, count, 0);
}

void sockets::Close(SocketDescriptor sockfd) {
  ::closesocket(sockfd);
}

int sockets::GetSocketError(SocketDescriptor sockfd) {
  int optval;
  int optlen = sizeof(optval);
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) < 0)
    return WSAGetLastError();
  return optval;
}

}  // namespace nev
