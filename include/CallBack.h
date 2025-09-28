#pragma once

#include <functional>
#include <memory>

namespace myMuduo
{
class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallBack = std::function<void(const TcpConnectionPtr &)>;
using CloseCallBack = std::function<void(const TcpConnectionPtr &)>;
using WriteCallBack = std::function<void(const TcpConnectionPtr &)>;
using MessageCallBack =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
} // namespace myMuduo