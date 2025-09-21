#pragma once

#include <iostream>

namespace myMuduo
{
    class Timestamp
    {
    public:
        Timestamp();
        explicit Timestamp(int64_t microSecondsSinceEpoch);
        // 获取当前时间
        static Timestamp now();
        // 时间转为string
        std::string toString() const;

    private:
        int64_t microSecondsSinceEpoch_;
    };
}