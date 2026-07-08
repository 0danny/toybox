#pragma once

#include <chrono>

class Timer
{
public:
    Timer();
    long long returnTime();
private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};