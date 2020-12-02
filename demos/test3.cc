#include "nev/nev_init.h"
#include "nev/event_loop.h"

using namespace nev;

int main(int argc, char* argv[]) {
  EnsureSocketInit();

  EventLoop loop;

  // 由于没有注册任何 watcher，循环马上退出。
  loop.loop();

  return 0;
}
