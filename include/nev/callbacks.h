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

// 连接建立回调
using ConnectionCallback = std::function<void(const TcpConnectionSharedPtr&)>;

// 收到数据回调
using MessageCallback = std::function<
    void(const TcpConnectionSharedPtr&, const char* data, ssize_t len)>;

// 连接断开回调
using CloseCallback = std::function<void(const TcpConnectionSharedPtr&)>;

}  // namespace nev
