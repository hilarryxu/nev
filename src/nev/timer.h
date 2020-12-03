#pragma once

#include "base/atomicops.h"

#include "nev/nev_export.h"
#include "nev/non_copyable.h"
#include "nev/callbacks.h"

#include "nev/libev_prelude.h"

namespace nev {

class NEV_EXPORT Timer : NonCopyable {
 public:
  Timer(TimerCallback cb, ev_tstamp after, ev_tstamp repeat);
  ~Timer();

  int64_t sequence() const { return sequence_; }

  // 启动定时器
  void start(struct ev_loop* io_loop);

  // 定时器触发后调用用户回调函数
  void handleEvent(int revents);

 private:
  const TimerCallback timer_cb_;
  struct ev_timer timer_watcher_;
  // 目前就是 TimerId
  const int64_t sequence_;

  struct ev_loop* io_loop_;

  static base::subtle::Atomic64 s_num_created_;
};

}  // namespace nev
