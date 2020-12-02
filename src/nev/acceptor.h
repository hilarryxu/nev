#pragma once

#include <functional>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/socket.h"
#include "nev/channel.h"

namespace nev {

class EventLoop;
class IPEndPoint;

// 接受器，用于服务端接受连接
class NEV_EXPORT Acceptor : NonCopyable {
 public:
  typedef std::function<void(SocketDescriptor sockfd, const IPEndPoint&)>
      NewConnectionCallback;

  Acceptor(EventLoop* loop, const IPEndPoint& listen_addr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb) {
    new_connection_cb_ = cb;
  }

  bool listenning() const { return listenning_; }

  // 在端口上监听，并关注可读事件
  void listen();

 private:
  // 套接字可读时触发，调用 accept 接受新的客户端连接
  void handleRead();

  EventLoop* loop_;
  Socket accept_socket_;
  Channel accept_channel_;
  // 新的客户端连接到来时的回调函数
  NewConnectionCallback new_connection_cb_;
  bool listenning_;
};

}  // namespace nev