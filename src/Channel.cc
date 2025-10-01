#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

namespace myMuduo
{
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() {}

// Channel的tie方法什么时候调用?
// 一个新的TcpConnection新连接创建的时候，调用该方法
// TcpConnection绑定了一个channel，使用弱指针指向tcpconnection对象，避免tcpconnection不存在了，而channel仍存在的情况
// 避免channel在tcpconnection不存在的情况下，自行调用回调方法
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 当改变channel的events时
// 将感兴趣的事件更新到Poller，让其epoll_ctl监听
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中，把当前的channel删除掉
void Channel::remove() { loop_->removeChannel(this); }

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的事件执行相应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("Channel handleEvent revents:%d\n", revents_);
    // 关闭
    /*
    EPOLLHUP表示对端已经关闭连接（如TCP连接中收到FIN包）。
    但是，当对端关闭连接时，通常也会触发可读事件（因为需要读取FIN包）。
    因此，这个条件判断的是“纯粹的挂断”，即没有数据可读的挂断（这种情况较少见）。
    常见的对端关闭连接会触发可读事件（读取到EOF），所以这里用&& !(revents_ &
    EPOLLIN)来避免重复处理。
    */
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallBack_)
        {
            closeCallBack_();
        }
    }

    // 发生错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallBack_)
        {
            errorCallBack_();
        }
    }

    // 读事件
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallBack_)
        {
            readCallBack_(receiveTime);
        }
    }

    // 写事件
    if (revents_ & EPOLLOUT)
    {
        if (writeCallBack_)
        {
            writeCallBack_();
        }
    }
}
} // namespace myMuduo