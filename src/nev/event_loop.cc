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

}  // namespace

class EventLoop::Impl {
 public:
  Impl() { loop_ = static_cast<struct ev_loop*>(ev_loop_new(EVFLAG_AUTO)); }

  ~Impl() { ev_loop_destroy(loop_); }

  struct ev_loop* loop() {
    return loop_;
  }

 private:
  struct ev_loop* loop_;
};

EventLoop::EventLoop()
    : impl_(std::make_unique<Impl>()),
      looping_(false),
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
  ev_run(impl_->loop(), 0);

  LOG(DEBUG) << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  ev_break(impl_->loop(), EVBREAK_ALL);
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
  ev_io_stop(impl_->loop(), io_watcher);
  ev_io_set(io_watcher, fd, events);
  // 不关注任何事件就不必再 start 了
  if (!channel->isNoneEvent())
    ev_io_start(impl_->loop(), io_watcher);
}

void EventLoop::removeChannel(Channel* channel) {
  DCHECK(channel->loop() == this);
  assertInLoopThread();

  struct ev_io* io_watcher = channel->io_watcher();
  ev_io_stop(impl_->loop(), io_watcher);
}

void EventLoop::abortNotInLoopThread() {
  // 断言失败，打印日志并退出进程。
  LOG(FATAL) << "EventLoop::abortNotInLoopThread - EventLoop " << this
             << " was created in threadId_ = " << thread_id_
             << ", current thread id = " << base::PlatformThread::CurrentId();
}

}  // namespace nev
