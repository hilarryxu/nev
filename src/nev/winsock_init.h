// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Winsock initialization must happen before any Winsock calls are made.  The
// EnsureWinsockInit method will make sure that WSAStartup has been called.

#pragma once

#include "nev/nev_export.h"

namespace nev {

// Make sure that Winsock is initialized, calling WSAStartup if needed.
NEV_EXPORT void EnsureWinsockInit();

}  // namespace nev
