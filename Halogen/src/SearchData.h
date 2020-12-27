#pragma once
#include "Position.h"
#include "TimeManage.h"
#include "EvalCache.h"
#include "TranspositionTable.h"

extern TranspositionTable tTable;

enum Score
{
	HighINF = 30000,
	LowINF = -30000,

	MATED = -10000,
	TB_LOSS_SCORE = -5000,
	DRAW = 0,
	TB_WIN_SCORE = 5000,
	MATE = 10000
};

class SearchLimits
{
public:
	SearchLimits() : SearchTimeManager(0, 0) {};

	bool CheckTimeLimit() const;
	bool CheckDepthLimit(int depth) const;
	bool CheckMateLimit(int score) const;
	bool CheckContinueSearch() const;

	void SetTimeLimits(int maxTime, int allocatedTime);
	void SetInfinite();
	void SetDepthLimit(int depth);
	void SetMateLimit(int moves);

	int ElapsedTime() {	return SearchTimeManager.ElapsedMs(); }

private:

	SearchTimeManage SearchTimeManager;
	mutable int PeriodicCheck = 0;

	int depthLimit = -1;
	int mateLimit = -1;
	bool IsInfinite = false;

	bool timeLimitEnabled = false;
	bool depthLimitEnabled = false;
	bool mateLimitEnabled = false;

	mutable bool periodicTimeLimit = false;
};

struct SearchData
{
	SearchData(const SearchLimits& Limits);

//--------------------------------------------------------------------------------------------
private:
	uint64_t padding1[8] = {};	//To avoid false sharing between adjacent SearchData objects
//--------------------------------------------------------------------------------------------

public:
	std::vector<std::vector<Move>> PvTable;
	std::vector<std::array<Move, 2>> KillerMoves;							//2 moves indexed by distanceFromRoot
	unsigned int HistoryMatrix[N_PLAYERS][N_SQUARES][N_SQUARES];			//first index is from square and 2nd index is to square
	EvalCacheTable evalTable;
	SearchLimits limits;

	void AddNode() { nodes++; }
	void AddTbHit() { tbHits++; }

private:
	friend class ThreadSharedData;
	uint64_t tbHits = 0;
	uint64_t nodes = 0;

//--------------------------------------------------------------------------------------------
private:
	uint64_t padding2[8] = {};	//To avoid false sharing between adjacent SearchData objects
//--------------------------------------------------------------------------------------------
};

class ThreadSharedData
{
public:
	ThreadSharedData(const SearchLimits& limits, unsigned int threads = 1, bool NoOutput = false);
	~ThreadSharedData();

	Move GetBestMove();
	bool ThreadAbort(unsigned int initialDepth) const;
	void ReportResult(unsigned int depth, double Time, int score, int alpha, int beta, const Position& position, Move move, const SearchData& locals);
	void ReportDepth(unsigned int depth, unsigned int threadID);
	void ReportWantsToStop(unsigned int threadID);
	int GetAspirationScore();

	uint64_t getTBHits() const;
	uint64_t getNodes() const;

	SearchData& GetData(unsigned int threadID);

private:
	void PrintSearchInfo(unsigned int depth, double Time, bool isCheckmate, int score, int alpha, int beta, const Position& position, const Move& move, const SearchData& locals) const;

	std::mutex ioMutex;
	unsigned int threadCount;
	unsigned int threadDepthCompleted;				//The depth that has been completed. When the first thread finishes a depth it increments this. All other threads should stop searching that depth
	Move currentBestMove;							//Whoever finishes first gets to update this as long as they searched deeper than threadDepth
	int prevScore;									//if threads abandon the search, we need to know what the score was in order to set new alpha/beta bounds
	int lowestAlpha;
	int highestBeta;
	bool noOutput;									//Do not write anything to the concole

	std::vector<SearchData> threadlocalData;

	//TODO: probably put this inside of SearchData
	std::vector<unsigned int> searchDepth;			//what depth is each thread currently searching?
	std::vector<bool> ThreadWantsToStop;			//Threads signal here that they want to stop searching, but will keep going until all threads want to stop
};
