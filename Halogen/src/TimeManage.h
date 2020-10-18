#pragma once
#include <time.h>
#include <atomic>
#include <mutex>
#include <iostream>

extern std::atomic<bool> KeepSearching;

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

	clock_t Begin;
	clock_t End;
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

	bool CacheShouldStop = false;
};

