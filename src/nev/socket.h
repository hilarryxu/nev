#pragma once

#include "build/build_config.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/socket_descriptor.h"

#if defined(OS_WIN)
#include <io.h>
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) _open_osfhandle(handle, 0)
#else
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) handle
#endif

namespace nev {

class IPEndPoint;

// 封装套接字
// 析构时会关闭套接字
class NEV_EXPORT Socket : NonCopyable {
 public:
  explicit Socket(SocketDescriptor sockfd)
      : sockfd_(sockfd), fd_(EV_SOCKETDESCRIPTOR_TO_FD(sockfd)) {}

  ~Socket();

  SocketDescriptor sockfd() const { return sockfd_; }
  int fd() const { return fd_; }

  // abort if address in use
  void bindAddress(const IPEndPoint& localaddr);

  // abort if address in use
  void listen();

  // 成功返回有效的套接字，并且已经设置为非阻塞和 close-on-exec，
  // |peeraddr| 也会设置。
  // 失败时返回 kInvalidSocket，也不会去修改 peeraddr。
  SocketDescriptor accept(IPEndPoint* peeraddr, bool nonblocking = true);

  void shutdownWrite();

  // 控制 Nagle 算法是否开启
  void setTcpNoDelay(bool on);

  // 设置 SO_REUSEADDR 选项
  void setReuseAddr(bool on);

  // 设置 SO_REUSEPORT 选项
  void setReusePort(bool on);

  // 设置 SO_KEEPALIVE 选项
  void setKeepAlive(bool on);

 private:
  const SocketDescriptor sockfd_;
  int fd_;
};

}  // namespace nev
