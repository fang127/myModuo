#pragma once

#include <unordered_map>
#include <vector>

#include "Timestamp.h"
#include "noncopyable.h"

namespace myMuduo
{
    // 前置声明
    class Channel;
    class EventLoop;

    // 多路事件分发器的核心IO复用模块，负责事件监听
    class Poller : noncopyable
    {
    public:
        using ChannelList = std::vector<Channel *>;

        Poller(EventLoop *loop);

        virtual ~Poller() = default;

        // 给所有IO复用统一接口,利用多态
        virtual Timestamp poll(int timeoutMs, ChannelList *activateChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;

        // 判断参数channel是否在当前Poller当中
        bool hasChannel(Channel *channel) const;

        // EventLoop可以通过该接口获取默认的IO复用的具体实例化对象
        static Poller *newDefaultPoller(EventLoop *loop);

    protected:
        // key为sockfd，value为channel
        using ChannelMap = std::unordered_map<int, Channel *>;
        ChannelMap channels_;

    private:
        EventLoop *ownerLoop_; // Poller所属的事件循环EventLoop
    };
} // namespace myMuduo