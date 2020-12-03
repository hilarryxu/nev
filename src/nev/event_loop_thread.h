#pragma once

#include <string>
#include <functional>

#include "base/threading/platform_thread.h"
#include "base/synchronization/waitable_event.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"

namespace nev {

class EventLoop;

class NEV_EXPORT EventLoopThread : NonCopyable,
                                   public base::PlatformThread::Delegate {
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const std::string& name = std::string());
  ~EventLoopThread();

  // 启动线程并等待获取 loop
  EventLoop* startLoop();

  void ThreadMain() override;

 private:
  const std::string name_prefix_;
  EventLoop* loop_;  // GUARDED BY event_
  base::WaitableEvent event_;
  // 初始化回调，可以对 loop 做一些设置操作
  ThreadInitCallback callback_;
};

}  // namespace nev
