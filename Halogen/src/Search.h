#pragma once

#include "TranspositionTable.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Move.h"
#include "TimeManage.h"
#include "tbprobe.h"
#include <ctime>
#include <algorithm>
#include <thread>
#include <cmath>

/*Tuneable search constants*/

extern double LMR_constant;
extern double LMR_coeff;

extern int Null_constant;				
extern int Null_depth_quotent;				
extern int Null_beta_quotent;

extern int Futility_linear;
extern int Futility_constant;

extern int Aspiration_window;

extern int Delta_margin;

/*----------------*/

struct SearchResult
{
	SearchResult(int score, Move move = Move()) : m_score(score), m_move(move) {}
	~SearchResult() {}

	int GetScore() const { return m_score; }
	Move GetMove() const { return m_move; }

private:
	int m_score;
	Move m_move;
};

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

struct Killer
{
	Move move[2];
};

struct SearchData
{
	SearchData();

	std::vector<std::vector<Move>> PvTable;
	std::vector<Killer> KillerMoves;							//2 moves indexed by distanceFromRoot
	unsigned int HistoryMatrix[N_PLAYERS][N_SQUARES][N_SQUARES];			//first index is from square and 2nd index is to square
	EvalCacheTable evalTable;
	SearchTimeManage timeManage;

	uint64_t tbHits = 0;
	uint64_t nodes = 0;

	bool AbortSearch();
	bool ContinueSearch();
};

class ThreadSharedData
{
public:
	ThreadSharedData(unsigned int threads = 1, bool NoOutput = false);
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

extern TranspositionTable tTable;

void MultithreadedSearch(const Position& position, unsigned int maxTimeMs, unsigned int AllocatedTimeMs, unsigned int threadCount = 1, int maxSearchDepth = MAX_DEPTH);
uint64_t BenchSearch(const Position& position, int maxSearchDepth = MAX_DEPTH);
void DepthSearch(const Position& position, int maxSearchDepth);
void MateSearch(const Position& position, int searchTime, int mate);

