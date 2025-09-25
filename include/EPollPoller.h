#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <sys/epoll.h>
#include <vector>

namespace myMuduo
{
    class Channel;

    /*
        epoll的使用
        1. epoll_creat
        2. epoll_ctl       add/mod/del
        3. epoll_wait
    */
    class EPollPoller : public Poller
    {
    public:
        EPollPoller(EventLoop *loop);

        ~EPollPoller() override;

        // 重写基类Poller的抽象方法
        Timestamp poll(int timeoutMs, ChannelList *activateChannels) override;
        void updateChannel(Channel *channel) override;
        void removeChannel(Channel *channel) override;

    private:
        // 填写活跃的连接
        void fillActiveChannels(int numEvents, ChannelList *activateChannels) const;
        // 更新channel通道
        void update(int operation, Channel *channel);

        static const int kInitEventListSize = 16;

        using EventList = std::vector<epoll_event>;

        int epollfd_;
        EventList events_;
    };
}