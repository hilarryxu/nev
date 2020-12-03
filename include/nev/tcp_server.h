#pragma once

#include <string>
#include <map>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/tcp_connection.h"

namespace nev {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

// 封装 TCP 服务端
class NEV_EXPORT TcpServer : NonCopyable {
 public:
  TcpServer(EventLoop* loop, const IPEndPoint& listen_addr);
  ~TcpServer();

  // 设置 subReactors io 线程数
  void setThreadNum(int num_threads);

  // 启动服务
  // 目前不是线程安全的，待改进。
  void start();

  // 不是线程安全的
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  // 不是线程安全的
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  // 不是线程安全的
  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    write_complete_cb_ = cb;
  }

 private:
  // 监听套接字获取到新的客户端连接时回调
  // 在同一 acceptor loop 中是安全的
  void newConnection(SocketDescriptor sockfd, const IPEndPoint& peer_addr);
  // 连接断开时回调
  // 主 reactor 上提交到连接对应的 subReactor 上执行 removeConnectionInLoop
  void removeConnection(const TcpConnectionSharedPtr& conn);
  // 连接对应的 loop 上执行是线程安全的
  void removeConnectionInLoop(const TcpConnectionSharedPtr& conn);

  using ConnectionMap = std::map<std::string, TcpConnectionSharedPtr>;

  // acceptor 所在 loop
  EventLoop* loop_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> thread_pool_;

  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;

  bool started_;
  int next_conn_id_;
  // 存放 连接名称 -> 连接对象 映射
  ConnectionMap connections_;
};

}  // namespace nev
