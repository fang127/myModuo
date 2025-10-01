#pragma once
#include "Socket.h"
#include "noncopyable.h"
#include <Channel.h>

#include <functional>

namespace myMuduo
{
class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using NewConnectionCallBack = std::function<void(int, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool resuseport);
    ~Acceptor();

    void setNewConnectionCallBack(const NewConnectionCallBack &cb)
    {
        newConnectionCallBack_ = std::move(cb);
    }

    bool listening() const { return listening_; }

    void listen();

private:
    void handleRead();

    EventLoop *loop_; // Acceptor就是用户定义的那个baseloop_,也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallBack newConnectionCallBack_;
    bool listening_;
};
} // namespace myMuduo