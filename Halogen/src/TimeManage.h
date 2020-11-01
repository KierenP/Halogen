#pragma once
#include <time.h>
#include <atomic>
#include <mutex>
#include <iostream>

extern std::atomic<bool> KeepSearching;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static inline double get_time_point() {
	return (double)(GetTickCount());
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
	int ElapsedMs();

private:
	double ElapsedTime;

	double Begin;
	double End;
};

class SearchTimeManage
{
public:
	SearchTimeManage();
	~SearchTimeManage();

	bool ContinueSearch();	//Should I search to another depth, or stop with what ive got?
	bool AbortSearch(uint64_t nodes);		//should I attempt to stop searching right now? Nodes is passed because we only want to check the exact time every 1000 nodes or so

	void StartSearch(int maxTime, int allocatedTime);	//pass the allowed search time maximum in milliseconds

private:
	Timer timer;
	int AllocatedSearchTimeMS;
	int MaxTimeMS;
	int BufferTime;

	bool CacheShouldStop = false;
};

