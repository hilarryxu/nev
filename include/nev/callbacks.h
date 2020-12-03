#pragma once

#include <functional>
#include <memory>

#include "base/time/time.h"

namespace nev {

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class Buffer;
class TcpConnection;

using TcpConnectionSharedPtr = std::shared_ptr<TcpConnection>;

// 连接建立回调
using ConnectionCallback = std::function<void(const TcpConnectionSharedPtr&)>;

// 收到数据回调
using MessageCallback = std::function<
    void(const TcpConnectionSharedPtr&, Buffer*, base::TimeTicks)>;

// 连接断开回调
using CloseCallback = std::function<void(const TcpConnectionSharedPtr&)>;

// 应用层缓冲区数据发送完时回调
using WriteCompleteCallback =
    std::function<void(const TcpConnectionSharedPtr&)>;

// 定时器回调
using TimerCallback = std::function<void()>;

}  // namespace nev
