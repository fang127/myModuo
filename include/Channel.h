#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>
#include <memory>

namespace myMuduo
{
// 前置声明
class EventLoop;

// Channel 通道，封装了sockfd和其感兴趣的event，如epollin。epollout事件
// 还绑定了poller类返回的具体时间
class Channel : noncopyable
{
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);

    ~Channel();

    // 处理事件。fd得到poller通知以后，调用相应的回调方法来处理事件
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallBack(ReadEventCallBack cb)
    {
        readCallBack_ = std::move(cb);
    }

    void setWriteCallBack(EventCallBack cb) { writeCallBack_ = std::move(cb); }

    void setCloseCallBack(EventCallBack cb) { closeCallBack_ = std::move(cb); }

    void setErrorCallBack(EventCallBack cb) { errorCallBack_ = std::move(cb); }

    // 防止channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }

    int events() const { return events_; }

    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update(); // 将感兴趣的事件添加到epoll
    }

    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }

    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    bool isWriting() const { return events_ & kWriteEvent; }

    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }

    void set_index(int index) { index_ = index; }

    // one loop per thread
    EventLoop *ownerLoop() { return loop_; }

    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // Poller监听的对象
    // events_
    // revents_都是位掩码，表示fd这个连接，用户感兴趣的是读或者写或者读写都感兴趣，或者没有感兴趣的事。
    // revents_是调用poll实际返回的发生的事件
    // revents_中的事件一定是
    // events_的子集（或相等），因为操作系统只会通知用户关心的事件
    int events_;  // 注册fd感兴趣的事件
    int revents_; // Poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面可以获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallBack readCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack errorCallBack_;
};
} // namespace myMuduo