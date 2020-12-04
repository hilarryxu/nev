#pragma once

#include <functional>
#include <memory>

#include "nev/socket_descriptor.h"
#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/ip_endpoint.h"

namespace nev {

class Channel;
class EventLoop;

// 封装连接器
class NEV_EXPORT Connector : NonCopyable {
 public:
  using NewConnectionCallback = std::function<void(SocketDescriptor)>;

  Connector(EventLoop* loop, const IPEndPoint& server_addr);
  ~Connector();

  // 连接建立后以套接字为参数调用用户注册的回调函数
  void setNewConnectionCallback(NewConnectionCallback cb) {
    new_connection_cb_ = std::move(cb);
  }

  // 开始连接
  // Thread safe.
  void start();
  // 停止连接
  // Thread safe.
  void stop();

  const IPEndPoint& server_addr() const { return server_addr_; }

  // 仅内部使用
  void startInLoop();

 private:
  enum StateE { kDisconnected, kConnecting, kConnected };

  void setState(StateE s) { state_ = s; }
  void connect();
  void connecting(SocketDescriptor sockfd);
  SocketDescriptor removeAndResetChannel();
  void resetChannel();
  void handleWrite();

  EventLoop* loop_;
  IPEndPoint server_addr_;
  bool connect_;
  StateE state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_connection_cb_;
};

using ConnectorSharedPtr = std::shared_ptr<Connector>;

}  // namespace nev
