#pragma once

#include <string>
#include <map>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/tcp_connection.h"

namespace nev {

class Acceptor;
class EventLoop;

// 封装 TCP 服务端
class NEV_EXPORT TcpServer : NonCopyable {
 public:
  TcpServer(EventLoop* loop, const IPEndPoint& listen_addr);
  ~TcpServer();

  // 启动服务
  // 目前不是线程安全的，待改进。
  void start();

  // 不是线程安全的
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  // 不是线程安全的
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

 private:
  // 监听套接字获取到新的客户端连接时回调
  // 在同一 acceptor loop 中是安全的
  void newConnection(SocketDescriptor sockfd, const IPEndPoint& peerAddr);

  using ConnectionMap = std::map<std::string, TcpConnectionSharedPtr>;

  EventLoop* loop_;  // the acceptor loop
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;

  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;

  bool started_;
  int next_conn_id_;
  // 存放 连接名称 -> 连接对象 映射
  ConnectionMap connections_;
};

}  // namespace nev
