#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"
#include "nev/timer_id.h"

namespace nev {

class Channel;
class Timer;

class NEV_EXPORT EventLoop : NonCopyable {
 public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  // 执行循环
  // NOTE: 必须在创建它的线程上调用
  void loop();

  // 退出循环
  void quit();

  // 在 loop 线程中执行回调函数
  // 在其他线程调用会退化为 queueInLoop
  // Thread safe.
  void runInLoop(Functor cb);
  // 将回调函数加入队列并唤醒线程
  // Thread safe.
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  // TODO: runAt

  // 延迟 after 秒的定时器
  // Thread safe.
  TimerId runAfter(double after, TimerCallback cb);
  // 每 repeat 秒重复执行的定时器
  // Thread safe.
  TimerId runEvery(double repeat, TimerCallback cb);
  // 取消定时器
  // Thread safe.
  void cancel(TimerId timer_id);

  // 仅内部使用
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  void destroyTimer(TimerId timer_id);

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

  // 当前线程的 loop
  static EventLoop* Current();

  void handleWakeup();
  void doPendingFunctors();

 private:
  void abortNotInLoopThread();
  void addTimerInLoop(Timer* timer);
  void cancelTimerInLoop(TimerId timer_id);

  class Impl;
  std::unique_ptr<Impl> impl_;

  bool looping_;                   // FIXME(xcc): atomic
  bool quit_;                      // FIXME(xcc): atomic
  bool calling_pending_functors_;  // FIXME(xcc): atomic
  base::PlatformThreadId thread_id_;

  mutable base::Lock pending_functors_lock_;
  // 每次 io loop 处理完后处理该队列中的回调函数
  std::vector<Functor> pending_functors_;  // Guarded by pending_functors_lock_
};

}  // namespace nev
