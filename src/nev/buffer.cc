// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "nev/buffer.h"

#include "build/build_config.h"

#if defined(OS_POSIX)
#include <errno.h>
#include <sys/uio.h>
#endif
#include "nev/macros.h"
#include "nev/sockets_ops.h"

using namespace nev;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

#if defined(OS_POSIX)
ssize_t Buffer::readFd(SocketDescriptor fd, int* savedErrno) {
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::Readv(fd, vec, iovcnt);
  if (n < 0) {
    *savedErrno = errno;
  } else if (implicit_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}
#elif defined(OS_WIN)
ssize_t Buffer::readFd(SocketDescriptor fd, int* savedErrno) {
  char extrabuf[65536];
  const size_t writable = writableBytes();
  const ssize_t n = sockets::Read(fd, extrabuf, sizeof(extrabuf));
  if (n <= 0) {
    *savedErrno = WSAGetLastError();
  } else if (implicit_cast<size_t>(n) <= writable) {
    append(extrabuf, n);
  } else {
    append(extrabuf, n);
  }
  return n;
}
#endif
