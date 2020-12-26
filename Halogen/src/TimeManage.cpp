#include "TimeManage.h"

std::atomic<bool> KeepSearching;

Timer::Timer() : Begin(0)
{
}

Timer::~Timer()
{
}

void Timer::Start()
{
	Begin = get_time_point();
}

void Timer::Restart()
{
	Begin = get_time_point();
}

int Timer::ElapsedMs() const
{
	return (get_time_point() - Begin);
}

SearchTimeManage::SearchTimeManage(int maxTime, int allocatedTime) : timer(Timer())
{
	timer.Restart();
	timer.Start();
	AllocatedSearchTimeMS = allocatedTime;
	MaxTimeMS = maxTime;

	BufferTime = 100;	//TODO: Make this a constant somewhere and reduce its value
}

SearchTimeManage::~SearchTimeManage()
{
}

bool SearchTimeManage::ContinueSearch() const
{
	//if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort search early
	return (AllocatedSearchTimeMS == MaxTimeMS || timer.ElapsedMs() < AllocatedSearchTimeMS / 2);	
}

bool SearchTimeManage::AbortSearch() const
{
	return (timer.ElapsedMs() > (std::min)(AllocatedSearchTimeMS, MaxTimeMS - BufferTime));
}
