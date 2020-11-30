#pragma once

#define EV_STANDALONE
#define EV_MULTIPLICITY 1

#include "build/build_config.h"

#if defined(OS_POSIX)
#include "libev_config_posix.h"
#elif defined(OS_WIN)
#include "libev_config_win.h"
#endif

#include "ev.h"
