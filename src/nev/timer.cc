#include "nev/timer.h"

#include "nev/libev_prelude.h"

namespace nev {

namespace {

void handleTimerCb(struct ev_loop* io_loop, struct ev_timer* w, int revents) {
  Timer* timer = static_cast<Timer*>(w->data);
  timer->handleEvent(revents);
}

}  // namespace

base::subtle::Atomic64 Timer::s_num_created_ = 0;

Timer::Timer(TimerCallback cb, ev_tstamp after, ev_tstamp repeat)
    : timer_cb_(std::move(cb)),
      sequence_(base::subtle::NoBarrier_AtomicIncrement(&s_num_created_, 1)),
      io_loop_(nullptr) {
  ev_timer_init(&timer_watcher_, &handleTimerCb, after, repeat);
  timer_watcher_.data = this;
}

Timer::~Timer() {
  if (io_loop_) {
    ev_timer_stop(io_loop_, &timer_watcher_);
    io_loop_ = nullptr;
  }
}

void Timer::start(struct ev_loop* io_loop) {
  io_loop_ = io_loop;
  ev_timer_start(io_loop, &timer_watcher_);
}

void Timer::handleEvent(int revents) {
  if (timer_cb_)
    timer_cb_();
}

}  // namespace nev
