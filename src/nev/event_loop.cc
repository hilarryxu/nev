#include "nev/socket_descriptor.h"
#include "nev/event_loop.h"

#include "build/build_config.h"
#include "base/logging.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

#include "nev/libev_prelude.h"
#include "nev/channel.h"

#if defined(OS_WIN)
#include <io.h>
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) _open_osfhandle(handle, 0)
#else
#define EV_SOCKETDESCRIPTOR_TO_FD(handle) handle
#endif

namespace nev {

namespace {

// 线程局部存储，保存线程对应的 EventLoop 指针。
// 前面的 LazyInstance 暂且不必关心。
base::LazyInstance<base::ThreadLocalPointer<EventLoop> >::Leaky lazy_tls_ptr =
    LAZY_INSTANCE_INITIALIZER;

void ioWatcherCb(struct ev_loop* loop, struct ev_io* io, int revents) {
  Channel* channel = static_cast<Channel*>(io->data);
  channel->set_revents(revents);
  channel->handleEvent();
}

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
  SocketDescriptor fd = channel->fd();
  int events = channel->events();
  io_watcher->data = channel;
  ev_io_init(io_watcher, &ioWatcherCb, EV_SOCKETDESCRIPTOR_TO_FD(fd), events);
  ev_io_start(impl_->loop(), io_watcher);
}

void EventLoop::abortNotInLoopThread() {
  // 断言失败，打印日志并退出进程。
  LOG(FATAL) << "EventLoop::abortNotInLoopThread - EventLoop " << this
             << " was created in threadId_ = " << thread_id_
             << ", current thread id = " << base::PlatformThread::CurrentId();
}

}  // namespace nev
