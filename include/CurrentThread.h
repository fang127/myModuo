#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace myMuduo::CurrentThread
{
extern thread_local int t_cachedTid;

void cacheTid();

inline int tid()
{
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
        cacheTid();
    }
    return t_cachedTid;
}
} // namespace myMuduo::CurrentThread