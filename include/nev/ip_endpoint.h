// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>  // uint16_t

#include <string>

#include "base/compiler_specific.h"

#include "nev/nev_export.h"
#include "nev/sys_addrinfo.h"
#include "nev/ip_address.h"

struct sockaddr;

namespace nev {

class NEV_EXPORT IPEndPoint {
 public:
  IPEndPoint();
  ~IPEndPoint();
  IPEndPoint(const IPAddress& address, uint16_t port);
  IPEndPoint(const IPEndPoint& endpoint);

  const IPAddress& address() const { return address_; }
  uint16_t port() const { return port_; }

  // 转换成 sockaddr 结构
  // |address| 存储空间大小至少为 sizeof(struct sockaddr_storage)
  // |address_length|
  //    输入：address 存储空间大小
  //    输出：实际拷贝至 address 的字节数
  bool toSockAddr(struct sockaddr* address,
                  socklen_t* address_length) const WARN_UNUSED_RESULT;

  // 从 sockaddr 结构体初始化
  bool fromSockAddr(const struct sockaddr* address,
                    socklen_t address_length) WARN_UNUSED_RESULT;

  // 正常返回类似 "127.0.0.1:80" 的字符串，当IP地址非法时返回空串。
  std::string toString() const;

  bool operator<(const IPEndPoint& that) const;
  bool operator==(const IPEndPoint& that) const;

 private:
  IPAddress address_;
  uint16_t port_;
};

}  // namespace nev
