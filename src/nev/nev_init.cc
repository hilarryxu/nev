#include "nev/nev_init.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include "nev/winsock_init.h"
#endif

namespace nev {

void EnsureSocketInit() {
#if defined(OS_WIN)
  EnsureWinsockInit();
#endif  // OS_WIN
}

}  // namespace nev
