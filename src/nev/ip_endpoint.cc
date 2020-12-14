// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "nev/ip_endpoint.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <winsock2.h>
#elif defined(OS_POSIX)
#include <netinet/in.h>
#endif

#include <tuple>

#include "base/logging.h"
#include "base/sys_byteorder.h"

namespace nev {

namespace {

const socklen_t kSockaddrInSize = sizeof(struct sockaddr_in);

// Extracts the address and port portions of a sockaddr.
bool GetIPAddressFromSockAddr(const struct sockaddr* sock_addr,
                              socklen_t sock_addr_len,
                              const uint8_t** address,
                              size_t* address_len,
                              uint16_t* port) {
  if (sock_addr->sa_family == AF_INET) {
    if (sock_addr_len < static_cast<socklen_t>(sizeof(struct sockaddr_in)))
      return false;
    const struct sockaddr_in* addr =
        reinterpret_cast<const struct sockaddr_in*>(sock_addr);
    *address = reinterpret_cast<const uint8_t*>(&addr->sin_addr);
    *address_len = IPAddress::kIPv4AddressSize;
    if (port)
      *port = base::NetToHost16(addr->sin_port);
    return true;
  }

  return false;  // Unrecognized |sa_family|.
}

}  // namespace

IPEndPoint::IPEndPoint() : port_(0) {}

IPEndPoint::~IPEndPoint() {}

IPEndPoint::IPEndPoint(const IPAddress& address, uint16_t port)
    : address_(address), port_(port) {}

IPEndPoint::IPEndPoint(const IPEndPoint& other) = default;

IPEndPoint& IPEndPoint::operator=(const IPEndPoint& other) = default;

bool IPEndPoint::toSockAddr(struct sockaddr* address,
                            socklen_t* address_length) const {
  DCHECK(address);
  DCHECK(address_length);
  switch (address_.size()) {
    case IPAddress::kIPv4AddressSize: {
      if (*address_length < kSockaddrInSize)
        return false;
      *address_length = kSockaddrInSize;
      struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(address);
      memset(addr, 0, sizeof(struct sockaddr_in));
      addr->sin_family = AF_INET;
      addr->sin_port = base::HostToNet16(port_);
      memcpy(&addr->sin_addr, address_.bytes().data(),
             IPAddress::kIPv4AddressSize);
      break;
    }
    default:
      return false;
  }
  return true;
}

bool IPEndPoint::fromSockAddr(const struct sockaddr* sock_addr,
                              socklen_t sock_addr_len) {
  DCHECK(sock_addr);
  const uint8_t* address;
  size_t address_len;
  uint16_t port;
  if (!GetIPAddressFromSockAddr(sock_addr, sock_addr_len, &address,
                                &address_len, &port)) {
    return false;
  }

  address_ = nev::IPAddress(address, address_len);
  port_ = port;
  return true;
}

std::string IPEndPoint::toString() const {
  return IPAddressToStringWithPort(address_, port_);
}

bool IPEndPoint::operator<(const IPEndPoint& other) const {
  if (address_.size() != other.address_.size()) {
    return address_.size() < other.address_.size();
  }
  return std::tie(address_, port_) < std::tie(other.address_, other.port_);
}

bool IPEndPoint::operator==(const IPEndPoint& other) const {
  return address_ == other.address_ && port_ == other.port_;
}

}  // namespace nev
