#pragma once
#include <time.h>

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
	bool AbortSearch();		//should I attempt to stop searching right now?

	//either tell it the allowed time then start it, or call start and give it then
	void StartSearch(int ms);

private:
	Timer timer;
	int AllowedSearchTimeMS;
};

