#pragma once

#include <stddef.h>  // ssize_t

#include <functional>
#include <memory>

namespace nev {

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class TcpConnection;

using TcpConnectionSharedPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionSharedPtr&)>;

using MessageCallback = std::function<
    void(const TcpConnectionSharedPtr&, const char* data, ssize_t len)>;

}  // namespace nev
