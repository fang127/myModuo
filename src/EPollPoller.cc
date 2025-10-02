#include "EPollPoller.h"
#include "Channel.h"
#include "Logger.h"

#include <errno.h>
#include <strings.h>
#include <unistd.h>

// channel的成员index_=-1，通过对比可表示channel在Poller中状态
// channel未添加到poller中
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

namespace myMuduo
{
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d\n", errno);
    }
}

EPollPoller::~EPollPoller() { ::close(epollfd_); }

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activateChannels)
{
    // 使用LOG_DEBUG，只有在DEBUG时候才会输出日志，避免正式使用的时候高并发时poll性能慢
    LOG_INFO("func=%s => fd total count:%lu\n", __func__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                                 static_cast<int>(events_.size()), timeoutMs);
    int saveError =
        errno; // errno是全局变量,也是每个线程独有的，可能被后续操作覆盖，因此需要保存到局部变量
               // saveError中。
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activateChannels);
        if (numEvents == events_.size())
        {
            /*
            epoll_wait的核心逻辑是：​预先分配一个足够大的数组（由
            maxevents参数指定长度），用于存储本次轮询中触发的事件。
            如果实际触发的事件数超过
            maxevents，超出的部分会被丢弃（不会被记录到数组中）。
            epoll_wait并不会主动向 events_中添加元素——它只是将事件写入
            events_预先分配的内存中。 因此，events_的大小（容量）需要在调用
            epoll_wait前就确定，无法通过 vector的自动扩容机制动态调整。
            */
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __func__);
    }
    else
    {
        if (saveError != EINTR)
        {
            errno = saveError; // 恢复 errno，确保日志或上层能正确获取错误码
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// epoll_ctl
// channel update remove =》 EventLoop updateChannel removeChannel =》 Poller
// updateChannel removeChannel
/*
    EventLoop包含ChannelList和Poller
    一个Channel是一个fd，ChannelList表示当前所有fd
    POller包含一个ChannelMap<fd,channel*>，即已被注册到epoll的fd
*/
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d\n", __func__, channel->fd(),
             channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel); // 注册channel到epoll
    }
    else // channel已在poller上注册过
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// epoll_ctl
// 从Poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    LOG_INFO("func=%s => fd=%d\n", __func__, fd);

    channels_.erase(fd);

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList *activateChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        // 获取fd
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activateChannels->push_back(
            channel); // Eventloop拿到它的Poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道
/*
    当改变channel所表示的fd的events事件后，update负责把相应事件更新到epoll_ctl
*/
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    int fd = channel->fd();
    event.data.fd = fd;
    /*
        data.ptr的核心作用是将 I/O 事件与用户自定义的上下文数据关联起来。
        在使用 epoll_ctl向 epoll 实例注册事件时，
        用户可以将任意类型的指针（如自定义结构体、对象等）存入 ptr字段。
        当该 I/O 对象触发事件（如可读、可写）时，
        通过 epoll_wait获取到的 epoll_event结构体中，
        data.ptr会保留注册时设置的指针，从而可以直接访问关联的上下文数据。
    */
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_ERROR("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
} // namespace myMuduo