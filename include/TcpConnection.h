#pragma once

#include "Buffer.h"
#include "CallBack.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <atomic>
#include <memory>
#include <string>

namespace myMuduo
{
class Channel;
class EventLoop;
class Socket;

/*
    TcpServer通过Acceptor的listing监听新连接，通过accept函数拿到connfd
    然后打包为TcpConnection设置Channel的回调，然后注册到Poller上，Poller监听已有连接
    有事件发生执行Channel的回调
*/
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getloop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);

    // 关闭服务器的连接
    void shutdown();

    // 设置回调
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
    void setHighWaterMarkCallBack(const HighWaterMarkCallBack &cb)
    {
        highWaterMarkCallBack_ = cb;
    }
    void setCloseCallBack(const CloseCallBack &cb) { closeCallBack_ = cb; }

    // 建立连接
    void connectEstablished();
    // 连接销毁
    void connectDestoryed();

private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);

    void shutdownInLoop();

    // 连接状态
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void setState(StateE state) { state_ = state; }

    EventLoop *loop_; // subloop
    const std::string name_;
    std::atomic<int> state_;
    bool reading_;

    // 这里和Acceptor类似       Acceptor=>mainLoop        TcpConnection=>subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallBack connectionCallBack_;       // 有新连接的回调
    MessageCallBack messageCallBack_;             // 有可读写消息的回调
    WriteCompleteCallBack writeCompleteCallBack_; // 消息发送完成后的回调
    HighWaterMarkCallBack highWaterMarkCallBack_; // 高水位回调
    CloseCallBack closeCallBack_;

    size_t highWaterMark_; // 避免发送的太快，而接收太慢造成队头阻塞

    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};

} // namespace myMuduo