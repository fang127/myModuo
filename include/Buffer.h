#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace myMuduo
{
/*
    网络库底层的缓冲区定义
*/
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 记录数据包的长度
    static const size_t kInitialSize = 1024; // 缓冲区大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    char *beginWrite() { return begin() + writerIndex_; }

    const char *beginWrite() const { return begin() + writerIndex_; }

    // onMessage string <- Buffer
    // 将缓冲区复位
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            /*
            应用只读取了可读缓冲区数据的一部分长度为len的数据，还剩下的就是
            readerIndex_+= len到writerIndex_
            */
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll() { readerIndex_ = writerIndex_ = kCheapPrepend; }

    // 将onMessage函数上报的buffer数据，转为string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        // 将缓冲区中可读的数读取到result中
        std::string result(peek(), len);
        // 将缓冲区进行复位操作
        retrieve(len);
        return result;
    }

    // buffer_.size() - writerIndex_
    // 检查是否有足够空间写
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            // 扩容
            makeSpace(len);
        }
    }

    // 添加数据,把data，data+len内存内的数据添加到writable缓冲区中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }
    void makeSpace(size_t len)
    {
        // 总空间不足（含头部预留）
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        // 空间足够但碎片化
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace myMuduo