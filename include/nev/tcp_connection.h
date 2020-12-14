#pragma once

#include <memory>
#include <string>

#include "base/strings/string_piece.h"

#include "nev/socket_descriptor.h"
#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"
#include "nev/ip_endpoint.h"
#include "nev/buffer.h"

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
  bool disconnected() const { return state_ == kDisconnected; }

  // 发送数据
  // Thread safe.
  void send(const void* data, size_t len);
  void send(const base::StringPiece& message);
  void send(Buffer* buf);
  // 主动关闭连接（其实是写完后关闭写端）
  // Not Thread safe.
  void shutdown();
  void setTcpNoDelay(bool on);

  // TODO: Context 支持

  // 连接建立并设置首次关注可读事件后回调
  void setConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }
  // 收到数据时回调
  void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }
  // 应用层缓冲区数据发送完时回调
  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    write_complete_cb_ = cb;
  }
  // 堆积数据太多时回调
  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                size_t high_water_mark) {
    high_water_mark_cb_ = cb;
    high_water_mark_ = high_water_mark;
  }

  Buffer* inputBuffer() { return &input_buffer_; }

  Buffer* outputBuffer() { return &output_buffer_; }

  // 仅内部使用
  void setCloseCallback(const CloseCallback& cb) { close_cb_ = cb; }

  // TcpServer 接受新的客户端连接时调用
  void connectEstablished();
  // 从 TcpServer 的 map 中移除后调用
  void connectDestroyed();

 private:
  // TCP 连接状态
  // 初始状态：kConnecting
  // 连接建立后：kConnected
  // shutdown 后：kDisconnecting
  // 断开连接：kDisconnected
  enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected };

  void setState(StateE s) { state_ = s; }
  void handleRead(base::TimeTicks receive_time);
  void handleWrite();
  void handleClose();
  void handleError(int saved_errno);
  void sendInLoop(const base::StringPiece& message);
  void sendInLoop(const void* data, size_t len);
  void shutdownInLoop();

  // 一个 TcpConnection 只能属于一个 loop
  EventLoop* loop_;
  const std::string name_;
  StateE state_;  // FIXME(xcc): atomic
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const IPEndPoint local_addr_;
  const IPEndPoint peer_addr_;

  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;
  HighWaterMarkCallback high_water_mark_cb_;
  CloseCallback close_cb_;

  size_t high_water_mark_;
  Buffer input_buffer_;
  Buffer output_buffer_;
};

}  // namespace nev
