#pragma once

#include <functional>
#include <memory>

#include "nev/socket_descriptor.h"
#include "nev/nev_export.h"
#include "nev/non_copyable.h"

struct ev_io;

namespace nev {

class EventLoop;

class NEV_EXPORT Channel : NonCopyable {
 public:
  typedef std::function<void()> EventCallback;

  Channel(EventLoop* loop, SocketDescriptor fd);
  ~Channel();

  // 处理事件
  void handleEvent();

  void setReadCallback(EventCallback cb) { read_cb_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { write_cb_ = std::move(cb); }

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
  const SocketDescriptor fd_;
  // 关注的事件
  int events_;
  // poll 后待处理事件
  int revents_;

  // 读取数据回调函数
  EventCallback read_cb_;
  // 写入数据回调函数
  EventCallback write_cb_;
};

}  // namespace nev
