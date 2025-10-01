#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

namespace myMuduo
{
// 从fd上读取数据，Poller默认工作在LT模式
// Buffer缓冲区是有大小的，从fd上读数据的时候，是不知道tcp数据最终的大小的
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // 开辟一个栈上的内存空间
    char extrabuf[65536] = {0}; // 64kB
    struct iovec vec[2];
    // 获取可写空间大小
    const size_t writable = writableBytes();
    // 设置第一块缓冲区，将缓冲区可写的地址给第一块vec，这样先写入缓冲区，如果不够再写vec[1]
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    // 设置第二块缓冲区
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 一次最多读64kB
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    // Buffer缓冲区就够了的情况
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    // extrabuf也写了数据
    else
    {
        writerIndex_ = buffer_.size();
        // 将数据追加到vec[0]写入带缓冲区的数据后
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

} // namespace myMuduo