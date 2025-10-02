#include "Timestamp.h"

#include <time.h>

namespace myMuduo
{
Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

// 获取当前时间
Timestamp Timestamp::now() { return Timestamp(time(nullptr)); }

// 时间转为string
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    tm *tmTime = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", tmTime->tm_year + 1900,
             tmTime->tm_mon + 1, tmTime->tm_mday, tmTime->tm_hour,
             tmTime->tm_min, tmTime->tm_sec);

    return buf;
}
} // namespace myMuduo

// #include <iostream>

// int main()
// {
//     std::cout << myMuduo::Timestamp::now().toString() << std::endl;

//     return 0;
// }