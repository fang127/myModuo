#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

#include <errno.h>
#include <functional>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace myMuduo
{

static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __func__,
                  __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop)), name_(name), state_(kConnecting),
      reading_(true), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣的事件发生了，channel会回调相应的函数
    channel_->setReadCallBack(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));

    channel_->setWriteCallBack(std::bind(&TcpConnection::handleWrite, this));

    channel_->setCloseCallBack(std::bind(&TcpConnection::handleClose, this));

    channel_->setErrorCallBack(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(),
             channel_->fd(), static_cast<int>(state_));
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(&buf, buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this,
                                       buf.c_str(), buf.size()));
        }
    }
}

/*
发送数据,应用写的快，但内核发送数据慢，需要把待发送的数据写入缓冲区，而且设置了水位回调
*/
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection的shutdown，不能再进行发送了
    if (state_ == kDisconnecting)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }
    // 表示channel第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        // 发送成功
        if (nwrote >= 0)
        {
            // remaing > 0说明没有一次发送完，即可能data长度超过一次能发送的长度
            remaining = len - nwrote;
            // 一次性写入完成
            if (remaining == 0 && writeCompleteCallBack_)
            {
                // 既然一次性发送完成了数据，就不用给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallBack_, shared_from_this()));
            }
        }
        else // 发送出错
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                // 连接重置的请求
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 发送正常但没有一次发送完成,那么剩下的数据需要保存到缓冲区中，然后给channel注册epollout事件
    // poller发现tcp的发送缓冲区有空间，会通知相应的sock（channel）调用handlewrite回调方法
    // 最终调用TcpConnection::handleWrite方法，把发送缓冲区的数据全部发送完成
    if (!faultError && remaining > 0)
    {
        // 当前发送缓冲区剩余的待发送数据的长度
        size_t oldlen = outputBuffer_.readableBytes();
        if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ &&
            highWaterMarkCallBack_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallBack_,
                                         shared_from_this(),
                                         oldlen + remaining));
        }

        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 一定要打开channel的些事件，否则poller不会给channel通知epollout
            channel_->enableWriting();
        }
    }
}

// 建立连接
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 注册channel的读事件

    // 新连接建立，执行回调
    connectionCallBack_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestoryed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        // 把channel所有感兴趣的事件，从Poller中delete
        channel_->disableAll();
        connectionCallBack_(shared_from_this());
    }
    // 把channel从poller中删除掉
    channel_->remove();
}

// 关闭服务器的连接
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    // 说明当前outputBuff中的数据已经全部发送完全
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

    // 有可读事件发生，调用回调
    if (n > 0)
    {
        messageCallBack_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    // 断开
    else if (n = 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);

        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallBack_)
                {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallBack_, shared_from_this()));
                }
                // 在发送过程中调用了shutdown，要等待数据发送完成，在shutdown
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n",
                  channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), static_cast<int>(state_));
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    // 执行连接关闭回调
    connectionCallBack_(connPtr);
    // 执行关闭连接回调
    closeCallBack_(connPtr);
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen))
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",
              name_.c_str(), err);
}

} // namespace myMuduo