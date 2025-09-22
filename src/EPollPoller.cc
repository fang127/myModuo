#include "EPollPoller.h"
#include "Logger.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// channel未添加到poller中
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

namespace myMuduo
{
    EPollPoller::EPollPoller(EventLoop *loop)
        : Poller(loop),
          epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
          events_(kInitEventListSize)
    {
        if (epollfd_ < 0)
        {
            LOG_FATAL("epoll_create error:%d \n", errno);
        }
    }

    EPollPoller::~EPollPoller()
    {
        ::close(epollfd_);
    }
}