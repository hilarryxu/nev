#pragma once

#include <functional>
#include <memory>

#include "nev/socket_descriptor.h"
#include "base/time/time.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"

struct ev_io;

namespace nev {

class EventLoop;

class NEV_EXPORT Channel : NonCopyable {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(base::TimeTicks)>;

  Channel(EventLoop* loop, SocketDescriptor sockfd, int fd);
  ~Channel();

  // 处理事件
  void handleEvent(base::TimeTicks receive_time);

  void setReadCallback(ReadEventCallback cb) { read_cb_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { write_cb_ = std::move(cb); }

  SocketDescriptor sockfd() const { return sockfd_; }
  // NOTE: windwos 下套接字描述符和文件描述符是不一样的
  // libev 内部需要的是文件描述符，所以增加了这个函数
  SocketDescriptor fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revents) { revents_ = revents; }  // 仅内部使用
  // 判断是否不关注任何事件
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  // 开启关注读事件
  void enableReading() {
    events_ |= kReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  // 设置不关注任何事件
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }
  bool isWriting() const { return events_ & kWriteEvent; }

  EventLoop* loop() { return loop_; }

  // 仅内部使用
  struct ev_io* io_watcher();

 private:
  // 新增、修改、删除关注的事件
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  class Impl;
  std::unique_ptr<Impl> impl_;

  EventLoop* loop_;
  const SocketDescriptor sockfd_;
  const int fd_;
  // 关注的事件
  int events_;
  // poll 后待处理事件
  int revents_;

  // 是否正在处理事件
  bool event_handling_;

  // 读取数据回调函数
  ReadEventCallback read_cb_;
  // 写入数据回调函数
  EventCallback write_cb_;
};

}  // namespace nev
