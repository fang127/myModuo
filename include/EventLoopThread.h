#pragma once

#include "Thread.h"
#include "noncopyable.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace myMuduo
{
class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallBack = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallBack &cb = ThreadInitCallBack(),
                    const std::string &name = std::string());

    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallBack callback_;
};
} // namespace myMuduo