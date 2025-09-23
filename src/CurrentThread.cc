#include "CurrentThread.h"

namespace myMuduo::CurrentThread
{
    thread_local int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            // 通过linux系统调用获取当前线程pid值
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_getpid));
        }
    }
}