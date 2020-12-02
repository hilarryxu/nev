#pragma once

#include <stddef.h>  // ssize_t

#include <functional>
#include <memory>

namespace nev {

class TcpConnection;

using TcpConnectionSharedPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionSharedPtr&)>;

using MessageCallback = std::function<
    void(const TcpConnectionSharedPtr&, const char* data, ssize_t len)>;

}  // namespace nev
