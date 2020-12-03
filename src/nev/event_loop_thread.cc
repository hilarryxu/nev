#include "nev/socket_descriptor.h"
#include "nev/event_loop_thread.h"

#include "base/strings/stringprintf.h"

#include "nev/event_loop.h"

namespace nev {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const std::string& name)
    : name_prefix_(name), loop_(nullptr), event_(false, false), callback_(cb) {}

EventLoopThread::~EventLoopThread() {}

EventLoop* EventLoopThread::startLoop() {
  event_.Reset();
  // 创建并启动线程
  base::PlatformThread::CreateNonJoinable(0, this);

  EventLoop* loop = nullptr;
  // 等待 loop 初始化完毕
  event_.Wait();
  loop = loop_;

  return loop;
}

void EventLoopThread::ThreadMain() {
  const std::string name = base::StringPrintf(
      "%s/%d", name_prefix_.c_str(), base::PlatformThread::CurrentId());
  // Note: |name.c_str()| must remain valid for for the whole life of the
  // thread.
  base::PlatformThread::SetName(name);

  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  loop_ = &loop;
  event_.Signal();

  // io loop 循环
  loop.loop();

  loop_ = nullptr;

  // loop 结束后删除 EventLoopThread 自身
  // The EventLoopThread is non-joinable, so it deletes itself.
  delete this;
}

}  // namespace nev
