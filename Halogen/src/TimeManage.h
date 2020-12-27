#pragma once
#include <time.h>
#include <atomic>
#include <mutex>
#include <iostream>
#include <algorithm>

extern std::atomic<bool> KeepSearching;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static inline double get_time_point() {
	LARGE_INTEGER time, freq;
	(!QueryPerformanceFrequency(&freq));
	QueryPerformanceCounter(&time);
	return (double)time.QuadPart * 1000 / freq.QuadPart;
}
#else
#include <sys/time.h>
static inline double get_time_point() {

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
	Timer();
	~Timer();

	void Start();
	void Restart();
	int ElapsedMs() const;

private:
	double Begin;
};

class SearchTimeManage
{
public:
	SearchTimeManage(int maxTime, int allocatedTime);
	~SearchTimeManage();

	bool ContinueSearch() const;
	bool AbortSearch() const;		//Is the remaining time all used up?
	int ElapsedMs() const { return timer.ElapsedMs(); }

private:
	Timer timer;
	int AllocatedSearchTimeMS;
	int MaxTimeMS;
	int BufferTime;

	bool CacheShouldStop = false;
};

