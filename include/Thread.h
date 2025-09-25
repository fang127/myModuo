#pragma once
#include "noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace myMuduo
{
    class Thread
    {
    public:
        using ThreadFunc = std::function<void()>;

        explicit Thread(ThreadFunc func, const std::string &name = std::string());

        ~Thread();

        void start();
        void join();

        bool started() const
        {
            return started_;
        }
        pid_t tid() const
        {
            return tid_;
        }
        const std::string &name() const
        {
            return name_;
        }

        static int numCreated()
        {
            return numCreated_;
        }

    private:
        void setDefaultName();

        bool started_;
        bool joined_;
        std::shared_ptr<std::thread> thread_; // 指针控制thread对象，避免thread构造后立即执行
        pid_t tid_;
        ThreadFunc func_;
        std::string name_;                  // 线程名
        static std::atomic_int numCreated_; // 创建的线程个数
    };
} // namespace myMuduo::Thread