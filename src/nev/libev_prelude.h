#pragma once

#include "build/build_config.h"

#if defined(OS_WIN)
#include "libev/libev_config_win.h"
#elif defined(OS_POSIX)
#include "libev/config.h"
#endif

#include "libev/ev.h"
