#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>

#include "nev/nev_export.h"
#include "nev/non_copyable.h"

namespace nev {

class EventLoop;
class EventLoopThread;

class NEV_EXPORT EventLoopThreadPool : NonCopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThreadPool(EventLoop* base_loop, const std::string& name);
  ~EventLoopThreadPool();

  void setThreadNum(int num_threads) { num_threads_ = num_threads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // round-robin 算法获取 loop
  EventLoop* getNextLoop();

  // 按 hash_code 获取 loop
  EventLoop* getLoopForHash(size_t hash_code);

  // 返回所有 loop
  std::vector<EventLoop*> getAllLoops();

  bool started() const { return started_; }

  const std::string& name() const { return name_; }

 private:
  EventLoop* base_loop_;
  std::string name_;
  bool started_;
  int num_threads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};

}  // namespace nev
