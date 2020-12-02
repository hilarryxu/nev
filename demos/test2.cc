#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"

#include "nev/event_loop.h"

using namespace nev;

EventLoop* g_loop;

class MyThread : public base::SimpleThread {
 public:
  MyThread(const std::string& name_prefix) : base::SimpleThread(name_prefix) {}

  ~MyThread() override {}

  void Run() override { g_loop->loop(); }
};

int main(int argc, char* argv[]) {
  EventLoop loop;
  g_loop = &loop;

  MyThread thread("MyThread");
  thread.Start();
  thread.Join();

  return 0;
}
