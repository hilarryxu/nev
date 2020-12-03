#include "base/logging.h"
#include "base/time/time.h"

#include "nev/nev_init.h"
#include "nev/event_loop.h"

using namespace nev;

int cnt = 0;
EventLoop* g_loop;

void print(const char* msg) {
  LOG(DEBUG) << "msg " << base::Time::Now() << " " << msg;
  if (++cnt == 20) {
    g_loop->quit();
  }
}

int main(int argc, char* argv[]) {
  EnsureNevInit();

  EventLoop loop;
  g_loop = &loop;

  print("main");
  loop.runAfter(1, std::bind(print, "once1"));
  loop.runAfter(1.5, std::bind(print, "once1.5"));
  loop.runAfter(2.5, std::bind(print, "once2.5"));
  loop.runAfter(3.5, std::bind(print, "once3.5"));
  loop.runEvery(2, std::bind(print, "every2"));
  loop.runEvery(3, std::bind(print, "every3"));

  loop.loop();
  print("main loop exits");
}
