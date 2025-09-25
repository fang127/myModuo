#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"
#include "Poller.h"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>

namespace myMuduo
{
    // 防止一个线程创建多个EventLoop
    thread_local EventLoop *t_loopInThisThread = nullptr;

    // 定义默认的Poller IO复用接口的超时时间
    const int kPollTimeMs = 10000;

    // 创建wakeupfd，用来notify唤醒subReactor来处理新来的Channel
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_FATAL("eventfd error:%d \n", errno);
        }

        return evtfd;
    }

    EventLoop::EventLoop()
        : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()),
          poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
    {
        LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
        if (t_loopInThisThread)
        {
            LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
        }
        else
        {
            t_loopInThisThread = this;
        }

        // 设置wakeupfd的事件类型以及发生事件后的回调操作
        wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead, this));
        // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件
        wakeupChannel_->enableReading();
    }

    EventLoop ::~EventLoop()
    {
        wakeupChannel_->disableAll();
        wakeupChannel_->remove();
        ::close(wakeupFd_);
        t_loopInThisThread = nullptr;
    }

    void EventLoop::handleRead()
    {
        uint64_t one = 1;
        ssize_t n = read(wakeupFd_, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
        }
    }

    // 开启事件循环
    void EventLoop::loop()
    {
        looping_ = true;
        quit_ = false;
        LOG_INFO("EventLoop %p start looping \n", this);

        while (!quit_)
        {
            activateChannels_.clear();
            // 监听两类fd，一种是client的fd，一种是wakeupfd
            pollReturnTime_ = poller_->poll(kPollTimeMs, &activateChannels_);
            // 处理事件
            for (Channel *channel : activateChannels_)
            {
                // Poller监听的事件返回给EventLoop，处理监听到发生事件的fd
                channel->handleEvent(pollReturnTime_);
            }
            // 执行当前EWventLoop事件循环需要处理的回调操作
            /*
                IO线程 mainLoop accept fd 《=channel subLoop
                mainLoop事先注册一个回调cb（需要subloop执行）
                wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
            */
            doPendingFunctors();
        }
        LOG_INFO("EventLoop %p stop looping. \n", this);
    }
    /*
                        mainloop

                                        muduo库没有该模式   =================
       生产者-消费者的线程安全队列，mainloop监听连接请求fd，subloop监听已连接的fd
            muduo库是直接通过wakeupfd唤醒，通过轮询找到一个subloop，将连接事件fd置入其中
           subloop01    subloop02   subloop03
    */
    // 退出事件循环
    void EventLoop::quit()
    {
        quit_ = true;
        // 如果在其他线程中，调用quit，如在一个subloop调用mainloop的quit
        if (!isInLoopThread())
        {
            wakeup();
        }
    }

    // 在当前loop中执行cb
    void EventLoop::runInLoop(Functor cb)
    {
        if (isInLoopThread()) // 在当前的loop线程中，执行cb
        {
            cb();
        }
        else // 在非当前loop线程中执行cb,需要唤醒loop所在线程，执行cb
        {
            queueInLoop(cb);
        }
    }

    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void EventLoop::queueInLoop(Functor cb)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            pendingFunctors_.emplace_back(cb);
        }

        // 唤醒相应的，需要执行上面回调操作的loop的线程了
        // || callingPendingFunctors的意思是：当前loop正在执行回调时，又添加了新的回调，
        // loop()中，回调完了又会阻塞，所以唤醒
        if (!isInLoopThread() || callingPendingFunctors_)
        {
            wakeup(); // 唤醒loop所在线程
        }
    }

    // 唤醒loop所在的线程，用wakefd_写入一个数据,wakeupChannel就发生读事件，当前loop线程就会被唤醒
    void EventLoop::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = write(wakeupFd_, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
        }
    }

    // EventLoop的方法，调用Poller方法
    void EventLoop::updateChannel(Channel *channel)
    {
        poller_->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel *channel)
    {
        poller_->removeChannel(channel);
    }

    bool EventLoop::hasChannel(Channel *channel)
    {
        return poller_->hasChannel(channel);
    }

    // 执行回调
    void EventLoop::doPendingFunctors()
    {
        std::vector<Functor> functors;
        callingPendingFunctors_ = true;

        // 不妨碍别的进程向pendingFunctors写入回调
        {
            std::unique_lock<std::mutex> lock(mutex_);
            functors.swap(pendingFunctors_);
        }

        for (const Functor &functor : functors)
        {
            functor();
        }

        callingPendingFunctors_ = false;
    }
} // namespace myMuduo