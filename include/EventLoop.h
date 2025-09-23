#pragma once

#include "noncopyable.h"

#include <functional>

namespace myMuduo
{
    class Channel;
    class Poller;

    // 事件循环类
    // 主要包含两个模块 Channel(发生的事件)    Poller(epoll的抽象)
    class EventLoop : noncopyable
    {
    public:
        using Functor = std::function<void()>;

    private:
    };
} // namespace myMuduo