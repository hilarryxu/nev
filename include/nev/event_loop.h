#pragma once

#include <memory>

#include "base/threading/platform_thread.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"

namespace nev {

class Channel;

class NEV_EXPORT EventLoop : NonCopyable {
 public:
  EventLoop();
  ~EventLoop();

  // 执行循环
  // NOTE: 必须在创建它的线程上调用
  void loop();

  void quit();

  // 仅内部使用
  void updateChannel(Channel* channel);

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

  class Impl;
  std::unique_ptr<Impl> impl_;

  bool looping_;  // FIXME(xcc): atomic
  bool quit_;     // FIXME(xcc): atomic
  base::PlatformThreadId thread_id_;
};

}  // namespace nev
