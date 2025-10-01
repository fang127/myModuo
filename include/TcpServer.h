#pragma once

#include "Acceptor.h"
#include "CallBack.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "noncopyable.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace myMuduo
{
/*
    用户使用该类接口编写服务器程序，因此需要包含所需的所有头文件，避免用户逐个包含
*/

// 对外服务器编程类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallBack = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &name,
              Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallBack(const ThreadInitCallBack &cb)
    {
        threadInitCallBack_ = cb;
    }
    void setConnectionCallBack(const ConnectionCallBack &cb)
    {
        connectionCallBack_ = cb;
    }
    void setMessageCallBack(const MessageCallBack &cb)
    {
        messageCallBack_ = cb;
    }
    void setWriteCompleteCallBack(const WriteCompleteCallBack &cb)
    {
        writeCompleteCallBack_ = cb;
    }

    // 设置subloop个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;                    // 用户定义的loop，baseloop
    const std::string ipPort_;           // 服务器地址
    const std::string name_;             // 服务器名
    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop,监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallBack connectionCallBack_;       // 有新连接的回调
    MessageCallBack messageCallBack_;             // 有可读写消息的回调
    WriteCompleteCallBack writeCompleteCallBack_; // 消息发送完成后的回调

    ThreadInitCallBack threadInitCallBack_; // 线程初始化时的回调

    std::atomic_int started_;

    int nextConnId_;

    ConnectionMap connections_; // 保存所有的连接
};
} // namespace myMuduo