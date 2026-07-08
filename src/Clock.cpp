#include "Clock.hpp"

Timer::Timer()
	: startTime(std::chrono::high_resolution_clock::now())
{}

long long Timer::returnTime()
{
    long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
    return duration;
}