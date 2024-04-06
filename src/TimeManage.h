#pragma once
#include <atomic>
#include <time.h>

// TODO: move this into SearchSharedState
inline std::atomic<bool> KeepSearching;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

static inline double get_time_point()
{
    LARGE_INTEGER time, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart * 1000 / freq.QuadPart;
}
#else
#include <sys/time.h>

static inline double get_time_point()
{

    struct timeval tv;
    double secsInMilli, usecsInMilli;

    gettimeofday(&tv, NULL);
    secsInMilli = ((double)tv.tv_sec) * 1000;
    usecsInMilli = tv.tv_usec / 1000;

    return secsInMilli + usecsInMilli;
}
#endif

class Timer
{
public:
    Timer()
    {
        Reset();
    }

    int ElapsedMs() const;

    void Reset()
    {
        Begin = get_time_point();
    }

private:
    double Begin;
};

class SearchTimeManage
{
public:
    SearchTimeManage(int maxTime = 0, int allocatedTime = 0);

    bool ContinueSearch() const;
    bool AbortSearch() const; // Is the remaining time all used up?

    int ElapsedMs() const
    {
        return timer.ElapsedMs();
    }

    void Reset()
    {
        timer.Reset();
    }

private:
    Timer timer;
    int AllocatedSearchTimeMS;
    int MaxTimeMS;

    constexpr static int BufferTime = 100;
};
