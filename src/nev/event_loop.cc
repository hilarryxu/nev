#include "nev/socket_descriptor.h"
#include "nev/event_loop.h"

#include <unordered_map>

#include "build/build_config.h"
#include "base/logging.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

#include "nev/libev_prelude.h"
#include "nev/channel.h"
#include "nev/timer.h"

namespace nev {

using TimerPtr = std::unique_ptr<Timer>;

namespace {

// 线程局部存储，保存线程对应的 EventLoop 指针。
// 前面的 LazyInstance 暂且不必关心。
base::LazyInstance<base::ThreadLocalPointer<EventLoop> >::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

// 唤醒 IO 线程
void handleAsyncWakeup(struct ev_loop* io_loop, struct ev_async* w, int event) {
  EventLoop* loop = static_cast<EventLoop*>(ev_userdata(io_loop));
  loop->handleWakeup();
}

void handleDoPendingFunctors(struct ev_loop* io_loop,
                             struct ev_check* w,
                             int events) {
  EventLoop* loop = static_cast<EventLoop*>(ev_userdata(io_loop));
  loop->doPendingFunctors();
}

}  // namespace

class EventLoop::Impl {
 public:
  Impl(EventLoop* loop) {
    io_loop_ = static_cast<struct ev_loop*>(ev_loop_new(EVFLAG_AUTO));
    ev_set_userdata(io_loop_, loop);
    ev_async_init(&async_watcher_, &handleAsyncWakeup);
    ev_check_init(&check_watcher_, &handleDoPendingFunctors);
    ev_set_priority(&check_watcher_, EV_MINPRI);

    ev_async_start(io_loop_, &async_watcher_);
    ev_check_start(io_loop_, &check_watcher_);
  }

  ~Impl() {
    ev_check_stop(io_loop_, &check_watcher_);
    ev_async_stop(io_loop_, &async_watcher_);
    // 清除并销毁所有定时器
    timers_.clear();

    ev_loop_destroy(io_loop_);
  }

  using TimerMap = std::unordered_map<TimerId, TimerPtr>;

  struct ev_loop* io_loop_;
  struct ev_async async_watcher_;
  struct ev_check check_watcher_;

  TimerMap timers_;
};

EventLoop::EventLoop()
    : impl_(new Impl(this)),
      looping_(false),
      quit_(false),
      calling_pending_functors_(false),
      thread_id_(base::PlatformThread::CurrentId()) {
  LOG(DEBUG) << "EventLoop created " << this << " in thread " << thread_id_;

  if (lazy_tls_ptr.Pointer()->Get() != nullptr) {
    // 当前线程已经运行了一个 EventLoop，一个线程只能运行一个 EventLoop。
    LOG(FATAL) << "Another EventLoop " << lazy_tls_ptr.Pointer()->Get()
               << " exists in this thread " << thread_id_;
  } else {
    lazy_tls_ptr.Pointer()->Set(this);
  }
}

EventLoop::~EventLoop() {
  DCHECK(!looping_);
  lazy_tls_ptr.Pointer()->Set(nullptr);
}

void EventLoop::loop() {
  DCHECK(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  ev_run(impl_->io_loop_, 0);

  LOG(DEBUG) << "EventLoop " << this << " stop looping";
  looping_ = false;
}

// NOTE: 请不要在 loop() 前调用 quit()
void EventLoop::quit() {
  // 设置退出标志并尝试唤醒线程
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  } else {
    ev_break(impl_->io_loop_, EVBREAK_ALL);
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    base::AutoLock lock(pending_functors_lock_);
    pending_functors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || calling_pending_functors_) {
    wakeup();
  }
}

size_t EventLoop::queueSize() const {
  base::AutoLock lock(pending_functors_lock_);
  return pending_functors_.size();
}

TimerId EventLoop::runAfter(double after, TimerCallback cb) {
  Timer* timer = new Timer(std::move(cb), after, 0.0);
  runInLoop(std::bind(&EventLoop::addTimerInLoop, this, timer));
  return timer->sequence();
}

TimerId EventLoop::runEvery(double repeat, TimerCallback cb) {
  Timer* timer = new Timer(std::move(cb), repeat, repeat);
  runInLoop(std::bind(&EventLoop::addTimerInLoop, this, timer));
  return timer->sequence();
}

void EventLoop::cancel(TimerId timer_id) {
  // NOTE: 这里用 queueInLoop 是为了防止一次 loop 循环处理中
  // A、B 两个定时器同时触发时，A 定时器回调函数中取消了 B 定时器，
  // 导致 B 定时器回调函数访问的 Timer 对象已被销毁。
  // 缺点是一定程度上延后了取消动作（因为本来可以立马取消没到期的 C 定时器）。
  queueInLoop(std::bind(&EventLoop::cancelTimerInLoop, this, timer_id));
}

// 投递一个 async 事件，唤醒 io_loop。
// 解除底层的 ::poll 等待，得以处理 pending_functors_。
void EventLoop::wakeup() {
  ev_async_send(impl_->io_loop_, &impl_->async_watcher_);
}

void EventLoop::updateChannel(Channel* channel) {
  DCHECK(channel->loop() == this);
  assertInLoopThread();

  struct ev_io* io_watcher = channel->io_watcher();
  SocketDescriptor sockfd = channel->sockfd();
  int fd = channel->fd();
  int events = channel->events();
  LOG(DEBUG) << "EventLoop::updateChannel events = " << events
             << ", sockfd = " << sockfd << ", fd = " << fd;

  // FIXME(xcc): libev 是否有接口直接修改关注事件？
  // 这样不必每次 stop 再 start
  ev_io_stop(impl_->io_loop_, io_watcher);
  ev_io_set(io_watcher, fd, events);
  // 不关注任何事件就不必再 start 了
  if (!channel->isNoneEvent())
    ev_io_start(impl_->io_loop_, io_watcher);
}

void EventLoop::removeChannel(Channel* channel) {
  DCHECK(channel->loop() == this);
  assertInLoopThread();

  struct ev_io* io_watcher = channel->io_watcher();
  ev_io_stop(impl_->io_loop_, io_watcher);
}

void EventLoop::destroyTimer(TimerId timer_id) {
  assertInLoopThread();
  // 销毁 Timer
  size_t n = impl_->timers_.erase(timer_id);
  (void)n;
  DCHECK(n == 1);
}

// static
EventLoop* EventLoop::Current() {
  return lazy_tls_ptr.Pointer()->Get();
}

void EventLoop::handleWakeup() {
  // 处理 quit() 调用
  if (quit_) {
    ev_break(impl_->io_loop_, EVBREAK_ALL);
  }
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;

  {
    base::AutoLock lock(pending_functors_lock_);
    functors.swap(pending_functors_);
  }

  for (const Functor& functor : functors) {
    functor();
  }

  calling_pending_functors_ = false;
}

void EventLoop::addTimerInLoop(Timer* timer) {
  assertInLoopThread();
  TimerPtr timer_ptr(timer);
  // 存放到 map 中
  impl_->timers_[timer->sequence()] = std::move(timer_ptr);
  // 关联 loop 并启动定时器
  timer->start(this, impl_->io_loop_);
}

void EventLoop::cancelTimerInLoop(TimerId timer_id) {
  assertInLoopThread();
  // 从 map 中移除，会调用 Timer 的析构函数，停止定时器。
  // 这里不一定找得到，可能已经被取消过。
  impl_->timers_.erase(timer_id);
}

void EventLoop::abortNotInLoopThread() {
  // 断言失败，打印日志并退出进程。
  LOG(FATAL) << "EventLoop::abortNotInLoopThread - EventLoop " << this
             << " was created in threadId_ = " << thread_id_
             << ", current thread id = " << base::PlatformThread::CurrentId();
}

}  // namespace nev
