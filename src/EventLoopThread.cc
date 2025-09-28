#include "EventLoopThread.h"
#include "EventLoop.h"

#include <memory>

namespace myMuduo
{
EventLoopThread::EventLoopThread(const ThreadInitCallBack &cb,
                                 const std::string &name)
    : loop_(nullptr), exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name), cond_(),
      mutex_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock); // 条件变量，保证新线程的loop_
        }
        loop = loop_; // 将新创建的线程的loop返回给该主线程
    }

    return loop;
}

// 该方法是在单独大的新线程中进行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的EventLoop.和上面的线程是一一对应的，one
                    // loop per thread

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // 在新线程开启epoll_wait

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
} // namespace myMuduo