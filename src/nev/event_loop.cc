#include "nev/socket_descriptor.h"
#include "nev/event_loop.h"

#include "build/build_config.h"
#include "base/logging.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

#include "nev/libev_prelude.h"
#include "nev/channel.h"

namespace nev {

namespace {

// 线程局部存储，保存线程对应的 EventLoop 指针。
// 前面的 LazyInstance 暂且不必关心。
base::LazyInstance<base::ThreadLocalPointer<EventLoop> >::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

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
    ev_loop_destroy(io_loop_);
  }

  struct ev_loop* io_loop_;
  struct ev_async async_watcher_;
  struct ev_check check_watcher_;
};

EventLoop::EventLoop()
    : impl_(std::make_unique<Impl>(this)),
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

void EventLoop::quit() {
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

void EventLoop::handleWakeup() {
  if (quit_) {
    ev_break(impl_->io_loop_, EVBREAK_ALL);
    return;
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

void EventLoop::abortNotInLoopThread() {
  // 断言失败，打印日志并退出进程。
  LOG(FATAL) << "EventLoop::abortNotInLoopThread - EventLoop " << this
             << " was created in threadId_ = " << thread_id_
             << ", current thread id = " << base::PlatformThread::CurrentId();
}

}  // namespace nev
