#include "Acceptor.h"
#include "InetAddress.h"
#include "Logger.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace myMuduo
{
    static int createNonblocking()
    {
        int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (sockfd < 0)
        {
            LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __func__, __LINE__, errno);
        }
        return sockfd;
    }

    Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
        : loop_(loop),
          acceptSocket_(createNonblocking()),
          acceptChannel_(loop, acceptSocket_.fd()),
          listening_(false)
    {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(listenAddr);

        // 如果有新用户的连接，要执行一个回调，该回调将connfd => channel => subloop
        // baseloop => acceptChannel_(listenfd) =>
        acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead, this));
    }

    Acceptor::~Acceptor()
    {
        acceptChannel_.disableAll();
        acceptChannel_.remove();
    }

    void Acceptor::listen()
    {
        listening_ = true;
        acceptSocket_.listen();
        acceptChannel_.enableReading();
    }

    // listenfd有事件发生了，就是有用户连接了
    void Acceptor::handleRead()
    {
        InetAddress peerAddr;
        int connfd = acceptSocket_.accept(&peerAddr);
        if (connfd >= 0)
        {
            if (newConnectionCallBack_)
            {
                newConnectionCallBack_(connfd, peerAddr); // 轮询找到subloop，唤醒分发当前的新客户端channel
            }
            else
            {
                ::close(connfd);
            }
        }
        else
        {
            LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __func__, __LINE__, errno);
            if (errno == EMFILE)
            {
                LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __func__, __LINE__);
            }
        }
    }
}