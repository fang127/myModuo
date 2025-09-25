#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore>

namespace myMuduo
{
    std::atomic_int Thread::numCreated_ = 0;

    Thread::Thread(ThreadFunc func, const std::string &name)
        : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
    {
        setDefaultName();
    }

    Thread::~Thread()
    {
        if (started_ && !joined_)
        {
            thread_->detach(); // thread类提供了设置分离线程的方法
        }
    }

    // 一个Thread对象，记录的就是一个新线程的详细信息
    void Thread::start()
    {
        started_ = true;
        // 保证主线程和新线程的执行顺序​：新线程必须先完成 tid_的赋值，主线程才允许 start()
        // 返回。 这确保了 Thread对象的 tid_成员在 start() 后始终有效，避免了未初始化的问题。 sem_t sem; sem_init(&sem,
        // false, 0);
        std::binary_semaphore sem(0); // C++20
        // 开启线程
        thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                               {
                                                                   // 获取线程tid
                                                                   tid_ = CurrentThread::tid();
                                                                   // sem_post(&sem);
                                                                   sem.release();
                                                                   func_(); // 开启一个新线程，专门执行该线程函数
                                                               }));

        // 这里必须等待获取上面所创建的线程的tid值
        // sem_wait(&sem);
        sem.acquire();
    }

    void Thread::join()
    {
        joined_ = true;
        thread_->join();
    }

    void Thread::setDefaultName()
    {
        int num = ++numCreated_;
        if (name_.empty())
        {
            char buf[32] = {0};
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }
    }
} // namespace myMuduo::Thread