#pragma once

#include <memory>
#include <string>

#include "nev/socket_descriptor.h"
#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"
#include "nev/ip_endpoint.h"

namespace nev {

class Channel;
class EventLoop;
class Socket;

// 封装 TCP 连接
// 多个地方持有该对象，生命周期比较复杂，使用 std::shared_ptr 管理。
class NEV_EXPORT TcpConnection
    : NonCopyable,
      public std::enable_shared_from_this<TcpConnection> {
 public:
  TcpConnection(EventLoop* loop,
                const std::string& name,
                SocketDescriptor sockfd,
                const IPEndPoint& local_addr,
                const IPEndPoint& peer_addr);
  ~TcpConnection();

  EventLoop* loop() const { return loop_; }
  const std::string& name() const { return name_; }
  const IPEndPoint& local_addr() const { return local_addr_; }
  const IPEndPoint& peer_addr() const { return peer_addr_; }
  bool connected() const { return state_ == kConnected; }

  // 连接建立并设置首次关注可读事件后回调
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }
  // 收到数据时回调
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  // TcpServer 接受新的客户端连接时调用
  void connectEstablished();

 private:
  // TCP 连接状态
  // 初始状态：kConnecting
  // 连接建立后：kConnected
  enum StateE { kConnecting, kConnected };

  void setState(StateE s) { state_ = s; }
  void handleRead();

  EventLoop* loop_;
  const std::string name_;
  StateE state_;  // FIXME(xcc): atomic
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const IPEndPoint local_addr_;
  const IPEndPoint peer_addr_;

  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
};

}  // namespace nev