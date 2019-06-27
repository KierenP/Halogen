#include "TimeManage.h"

Timer::Timer()
{
	Begin = clock();
	End = clock();
	ElapsedTime = 0;
}

Timer::~Timer()
{
}

void Timer::Start()
{
	Begin = clock();
	End = clock();
}

void Timer::Restart()
{
	Begin = clock();
	End = clock();
	ElapsedTime = 0;
}

int Timer::ElapsedMs()
{
	End = clock();
	ElapsedTime = (double(End) - double(Begin)) / CLOCKS_PER_SEC * 1000;

	return ElapsedTime;
}

SearchTimeManage::SearchTimeManage()
{
	AllowedSearchTimeMS = 0;
	timer = Timer();
}

SearchTimeManage::~SearchTimeManage()
{
}

bool SearchTimeManage::ContinueSearch()
{
	return (timer.ElapsedMs() < AllowedSearchTimeMS / 2);
}

bool SearchTimeManage::AbortSearch()
{
	return (timer.ElapsedMs() > AllowedSearchTimeMS);
}

void SearchTimeManage::StartSearch(int ms)
{
	timer.Restart();
	AllowedSearchTimeMS = ms;
	timer.Start();
}
