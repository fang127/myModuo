#pragma once

namespace myMuduo
{
    /*
        禁止拷贝基类，派生类无法拷贝和赋值，可以构造和析构
    */
    class noncopyable
    {
    public:
        noncopyable(const noncopyable &) = delete;
        noncopyable &operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}