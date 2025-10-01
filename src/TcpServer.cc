#include "TcpServer.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"

#include <functional>

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

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {}

} // namespace myMuduo