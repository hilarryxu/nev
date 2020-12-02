#pragma once

#include "base/threading/platform_thread.h"

namespace nev {

class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

  // 执行循环
  // NOTE: 必须在创建它的线程上调用
  void loop();

  // 断言当前线程是否为 EventLoop 所在线程
  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  // 判断当前线程是否为 EventLoop 所在线程
  bool isInLoopThread() const {
    return thread_id_ == base::PlatformThread::CurrentId();
  }

 private:
  void abortNotInLoopThread();

  bool looping_;  // FIXME(xcc): atomic
  base::PlatformThreadId thread_id_;
};

}  // namespace nev
