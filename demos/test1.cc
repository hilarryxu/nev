#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"

#include "nev/event_loop.h"

using namespace nev;

class MyThread : public base::SimpleThread {
 public:
  MyThread(const std::string& name_prefix) : base::SimpleThread(name_prefix) {}

  ~MyThread() override {}

  void Run() override {
    printf("MyThread(): tid = %d\n", base::PlatformThread::CurrentId());

    EventLoop loop;
    loop.loop();
  }
};

int main(int argc, char* argv[]) {
  printf("main(): tid = %d\n", base::PlatformThread::CurrentId());

  EventLoop loop;

  MyThread thread("MyThread");
  thread.Start();

  loop.loop();

  thread.Join();

  return 0;
}
