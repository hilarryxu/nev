#include <stdio.h>

#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"

using namespace nev;

EventLoop* g_loop;

void run1() {
  printf("run1(): tid = %d\n", base::PlatformThread::CurrentId());
}

class MyThread : public base::SimpleThread {
 public:
  MyThread(const std::string& name_prefix) : base::SimpleThread(name_prefix) {}

  ~MyThread() override {}

  void Run() override {
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(5));
    g_loop->queueInLoop(run1);
    base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(3));
    g_loop->quit();
  }
};

int main(int argc, char* argv[]) {
  EnsureNevInit();

  EventLoop loop;
  g_loop = &loop;

  MyThread thread("MyThread");
  thread.Start();

  loop.loop();

  thread.Join();
}
