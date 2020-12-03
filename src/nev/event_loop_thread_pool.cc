#include "nev/event_loop_thread_pool.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "nev/macros.h"
#include "nev/event_loop.h"
#include "nev/event_loop_thread.h"

namespace nev {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop,
                                         const std::string& name)
    : base_loop_(base_loop),
      name_(name),
      started_(false),
      num_threads_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {
  // EventLoopThread 中的 loop 都是局部变量，无需在这里删除。
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
  DCHECK(!started_);
  base_loop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    const std::string name = base::StringPrintf("%s%d", name_.c_str(), i);
    EventLoopThread* t = new EventLoopThread(cb, name);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());
  }

  // 仅单个 base_loop 时
  if (num_threads_ == 0 && cb) {
    cb(base_loop_);
  }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
  base_loop_->assertInLoopThread();
  DCHECK(started_);
  EventLoop* loop = base_loop_;

  if (!loops_.empty()) {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hash_code) {
  base_loop_->assertInLoopThread();
  EventLoop* loop = base_loop_;

  if (!loops_.empty()) {
    loop = loops_[hash_code % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
  base_loop_->assertInLoopThread();
  DCHECK(started_);
  if (loops_.empty()) {
    // 返回单个 base_loop
    return std::vector<EventLoop*>(1, base_loop_);
  } else {
    return loops_;
  }
}

}  // namespace nev
