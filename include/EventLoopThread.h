#pragma once

#include "noncopyable.h"

#include <functional>

namespace myMuduo
{
    class EventLoop;

    class EventLoopThread : noncopyable
    {
    public:
        using ThreadInitCallBack = std::function<void(EventLoop *)>;

    private:
        EventLoop *loop_;
    };
} // namespace myMuoduo