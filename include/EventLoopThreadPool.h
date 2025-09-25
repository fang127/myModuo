#pragma once

#include "noncopyable.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace myMuduo
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : noncopyable
    {
    public:
        using ThreadInitCallBack = std::function<void(EventLoop *)>;

        EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
        ~EventLoopThreadPool();

        void setThread(int numThreads) { numThreads_ = numThreads; }

        void start(const ThreadInitCallBack &cb = ThreadInitCallBack());

        // 如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
        EventLoop *getNextLoop();

        std::vector<EventLoop *> getAllLoops();

        bool started() const { return started_; }

        const std::string name() const { return name_; }

    private:
        EventLoop *baseLoop_; // mainReactor
        std::string name_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop *> loops_;
    };
}