#pragma once

#include <functional>
#include <memory>

#include "nev/socket_descriptor.h"
#include "base/synchronization/lock.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/tcp_connection.h"

namespace nev {

class Connector;
using ConnectorSharedPtr = std::shared_ptr<Connector>;

// 封装 TCP 客户端
// TcpClient 的生命周期要短于其所属的 EventLoop
class NEV_EXPORT TcpClient : NonCopyable {
 public:
  TcpClient(EventLoop* loop, const IPEndPoint& server_addr);
  ~TcpClient();

  // 连接服务端
  void connect();
  // 主动断开连接（仅连接成功后调用才有效果）
  void disconnect();
  // 停止连接服务端
  void stop();

  void setConnectionCallback(ConnectionCallback cb) {
    connection_cb_ = std::move(cb);
  }

  void setMessageCallback(MessageCallback cb) { message_cb_ = std::move(cb); }

  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    write_complete_cb_ = std::move(cb);
  }

  // loop 中可能也在引用该连接对象，所以这里返回连接对象需要加锁。
  TcpConnectionSharedPtr connection() const {
    base::AutoLock lock(lock_);
    return connection_;
  }

 private:
  void newConnection(SocketDescriptor sockfd);
  void removeConnection(const TcpConnectionSharedPtr& conn);

  EventLoop* loop_;
  ConnectorSharedPtr connector_;
  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;
  bool connect_;
  int next_conn_id_;
  mutable base::Lock lock_;
  TcpConnectionSharedPtr connection_;  // Guarded by lock_
};

}  // namespace nev
