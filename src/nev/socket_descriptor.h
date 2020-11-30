// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "build/build_config.h"

#if defined(OS_WIN)
#include <winsock2.h>
#endif  // OS_WIN

#include "nev/nev_export.h"

namespace nev {

#if defined(OS_POSIX)
typedef int SocketDescriptor;
const SocketDescriptor kInvalidSocket = -1;
#elif defined(OS_WIN)
typedef SOCKET SocketDescriptor;
const SocketDescriptor kInvalidSocket = INVALID_SOCKET;
#endif

}  // namespace nev
