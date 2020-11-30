#pragma once

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/socket_descriptor.h"

namespace nev {

class IPEndPoint;

class NEV_EXPORT Socket : NonCopyable {
 public:
  explicit Socket(SocketDescriptor sockfd) : sockfd_(sockfd) {}

  ~Socket();

  SocketDescriptor fd() const { return sockfd_; }

  // abort if address in use
  void bindAddress(const IPEndPoint& localaddr);

  // abort if address in use
  void listen();

  // 成功返回有效的套接字，并且已经设置为非阻塞和 close-on-exec，
  // |peeraddr| 也会设置。
  // 失败时返回 kInvalidSocket，也不会去修改 peeraddr。
  SocketDescriptor accept(IPEndPoint* peeraddr);

 private:
  const SocketDescriptor sockfd_;
};

}  // namespace nev
