#include "Poller.h"

#include <stdlib.h>

namespace myMuduo
{
    // 为什么该静态成员单独放置在一个文件，是因为该方法要包含派生类头文件，基类的实现文件最好不应包含派生类头文件
    Poller *Poller::newDefaultPoller(EventLoop *loop)
    {
        if (::getenv("MUDUO_USE_POLL"))
        {
            return nullptr; // 生成poll实例
        }
        else
        {
            return nullptr; // 生成epoll实例
        }
    }

} // namespace myMuduo