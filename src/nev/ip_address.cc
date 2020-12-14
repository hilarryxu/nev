// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "nev/ip_address.h"

#include "base/strings/stringprintf.h"

namespace nev {

IPAddress::IPAddress() {}

IPAddress::IPAddress(const std::vector<uint8_t>& address)
    : ip_address_(address) {}

IPAddress::IPAddress(const uint8_t* address, size_t address_len)
    : ip_address_(address, address + address_len) {}

IPAddress::IPAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
  ip_address_.reserve(4);
  ip_address_.push_back(b0);
  ip_address_.push_back(b1);
  ip_address_.push_back(b2);
  ip_address_.push_back(b3);
}

IPAddress::IPAddress(const IPAddress& other) = default;

IPAddress& IPAddress::operator=(const IPAddress& other) = default;

IPAddress::~IPAddress() {}

bool IPAddress::isIPv4() const {
  return ip_address_.size() == kIPv4AddressSize;
}

std::string IPAddress::toString() const {
  if (isIPv4()) {
    return base::StringPrintf("%d.%d.%d.%d", ip_address_[0], ip_address_[1],
                              ip_address_[2], ip_address_[3]);
  }

  return "";
}

// static
IPAddress IPAddress::IPv4Localhost() {
  static const uint8_t kLocalhostIPv4[] = {127, 0, 0, 1};
  return IPAddress(kLocalhostIPv4);
}

bool IPAddress::operator==(const IPAddress& that) const {
  return ip_address_ == that.ip_address_;
}

bool IPAddress::operator!=(const IPAddress& that) const {
  return ip_address_ != that.ip_address_;
}

bool IPAddress::operator<(const IPAddress& that) const {
  if (ip_address_.size() != that.ip_address_.size()) {
    return ip_address_.size() < that.ip_address_.size();
  }

  return ip_address_ < that.ip_address_;
}

std::string IPAddressToStringWithPort(const IPAddress& address, uint16_t port) {
  std::string address_str = address.toString();
  if (address_str.empty())
    return address_str;

  return base::StringPrintf("%s:%d", address_str.c_str(), port);
}

}  // namespace nev
