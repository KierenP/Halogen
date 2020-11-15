#include "TimeManage.h"

std::atomic<bool> KeepSearching;

Timer::Timer() : Begin(0), End(0)
{
	ElapsedTime = 0;
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
	Begin = 0;
	End = 0;
	ElapsedTime = 0;
}

int Timer::ElapsedMs()
{
	End = get_time_point();
	ElapsedTime = (End - Begin);
	return ElapsedTime;
}

SearchTimeManage::SearchTimeManage() : timer(Timer())
{
	BufferTime = 100;
	AllocatedSearchTimeMS = 0;
	MaxTimeMS = 0;
}

SearchTimeManage::~SearchTimeManage()
{
}

bool SearchTimeManage::ContinueSearch()
{
	return (AllocatedSearchTimeMS == MaxTimeMS || timer.ElapsedMs() < AllocatedSearchTimeMS / 2);	//if AllocatedSearchTimeMS == MaxTimeMS then we have recieved a 'go movetime X' command and we should not abort search early
}

bool SearchTimeManage::AbortSearch(uint64_t nodes)
{
	if ((nodes & 0x3FF) == 0 || nodes <= 0x3FFF)	//will hit once every 1024, but every time its called initially to help with very fast time controls
		CacheShouldStop = (timer.ElapsedMs() > (AllocatedSearchTimeMS)) || (timer.ElapsedMs() > (MaxTimeMS - BufferTime));

	return (!KeepSearching || CacheShouldStop);
}

void SearchTimeManage::StartSearch(int maxTime, int allocatedTime)
{
	timer.Restart();
	timer.Start();
	AllocatedSearchTimeMS = allocatedTime;
	MaxTimeMS = maxTime;
}
