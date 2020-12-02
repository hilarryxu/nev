#pragma once

#include "nev/nev_export.h"
#include "nev/socket_descriptor.h"

namespace nev {

class IPEndPoint;

namespace sockets {

// 初始化套接字环境
void NEV_EXPORT EnsureSocketInit();

// 创建一个阻塞的套接字，失败直接退出程序。
SocketDescriptor NEV_EXPORT CreateOrDie();

// 创建一个非阻塞的套接字，失败直接退出程序。
SocketDescriptor NEV_EXPORT CreateNonblockingOrDie();

int NEV_EXPORT Connect(SocketDescriptor sockfd, const IPEndPoint& address);
void NEV_EXPORT BindOrDie(SocketDescriptor sockfd, const IPEndPoint& address);
void NEV_EXPORT ListenOrDie(SocketDescriptor sockfd);
SocketDescriptor NEV_EXPORT Accept(SocketDescriptor sockfd,
                                   IPEndPoint* address,
                                   bool nonblocking = true);
ssize_t NEV_EXPORT Read(SocketDescriptor sockfd, void* buf, size_t count);
ssize_t NEV_EXPORT Write(SocketDescriptor sockfd,
                         const void* buf,
                         size_t count);
void NEV_EXPORT Close(SocketDescriptor sockfd);

int NEV_EXPORT GetSocketError(SocketDescriptor sockfd);

}  // namespace sockets

}  // namespace nev
