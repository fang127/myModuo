#include "TcpServer.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <functional>
#include <string.h>

namespace myMuduo
{
static EventLoop *checkLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __func__,
                  __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &name,
                     Option option)
    : loop_(checkLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()),
      name_(name), acceptor_(new Acceptor(loop, listenAddr, option)),
      threadPool_(new EventLoopThreadPool(loop_, name_)), connectionCallBack_(),
      messageCallBack_(), nextConnId_(1)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallBack(std::bind(&TcpServer::newConnection,
                                                  this, std::placeholders::_1,
                                                  std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second); // 局部对象，出了作用域自动被析构
        item.second.reset(); // 释放Tcpserver中的TcpConnection指针
        conn->getloop()->runInLoop(
            std::bind(&TcpConnection::connectDestoryed, conn));
    }
}

// 设置subloop个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThread(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    // 防止一个tcpserver对象被start多次
    if (started_++ == 0)
    {
        // 启动底层线程池
        threadPool_->start(threadInitCallBack_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新的客户端的连接，会执行该回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法，选择一个subloop来管理该新连接的channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServe::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd绑定的服务器的ip地址和端口信息
    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);

    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen))
    {
        LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress localAddr(local);

    // 根据连接成功的sockfd,创建tcpConnection连接对象
    TcpConnectionPtr conn(
        new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

    connections_[connName] = conn;
    // 下面的回调都是用户设置给Tcpserver=》Tcpconnection=》Channel=》
    // Poller=》notify channel回调
    conn->setConnectionCallBack(connectionCallBack_);
    conn->setMessageCallBack(messageCallBack_);
    conn->setWriteCompleteCallBack(writeCompleteCallBack_);
    // 关闭连接的回调，不是由用户设置的
    conn->setCloseCallBack(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    // 直接调用
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
             name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getloop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}

} // namespace myMuduo