// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "nev/nev_export.h"

namespace nev {

class NEV_EXPORT IPAddress {
 public:
  enum : size_t { kIPv4AddressSize = 4, kIPv6AddressSize = 16 };

  // 零长度的非法地址
  IPAddress();

  // 需要网络字节序
  explicit IPAddress(const std::vector<uint8_t>& address);

  template <size_t N>
  IPAddress(const uint8_t (&address)[N]) : IPAddress(address, N) {}

  IPAddress(const uint8_t* address, size_t address_len);

  IPAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

  IPAddress(const IPAddress& other);

  ~IPAddress();

  bool isIPv4() const;

  size_t size() const { return ip_address_.size(); }

  const std::vector<uint8_t>& bytes() const { return ip_address_; };

  bool empty() const { return ip_address_.empty(); }

  // 返回类似 "192.168.0.1" 的字符串或者空串
  std::string toString() const;

  // 返回 127.0.0.1 IP地址
  static IPAddress IPv4Localhost();

  bool operator==(const IPAddress& that) const;
  bool operator!=(const IPAddress& that) const;
  bool operator<(const IPAddress& that) const;

 private:
  // 网络字节序的点分十进制
  std::vector<uint8_t> ip_address_;
};

// 返回类似 "192.168.0.1:99" 的字符串
NEV_EXPORT std::string IPAddressToStringWithPort(const IPAddress& address,
                                                 uint16_t port);

}  // namespace nev
