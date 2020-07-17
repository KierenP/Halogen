#pragma once

#include "TranspositionTable.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Move.h"
#include "TimeManage.h"
#include <ctime>
#include <algorithm>
#include <thread>

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

const unsigned int MAX_DEPTH = 100;

enum Score
{
	HighINF = 30000,
	LowINF = -30000,

	MateScore = -10000,
	Draw = 0
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
	SearchTimeManage timeManage;
};

class ThreadSharedData
{
public:
	ThreadSharedData(unsigned int threads = 1);
	~ThreadSharedData();

	Move GetBestMove();
	bool ThreadAbort(unsigned int initialDepth);
	void ReportResult(unsigned int depth, double Time, int score, int alpha, int beta, Position& position, Move move, SearchData& locals);
	int GetAspirationScore();

private:
	std::mutex ioMutex;
	unsigned int threadCount;
	unsigned int threadDepthCompleted;				//The depth that has been completed. When the first thread finishes a depth it increments this. All other threads should stop searching that depth
	Move currentBestMove;							//Whoever finishes first gets to update this as long as they searched deeper than threadDepth
	int prevAlpha;
	int prevBeta;
	int prevScore;									//if threads abandon the search, we need to know what the score was in order to set new alpha/beta bounds
};

extern TranspositionTable tTable;

Move MultithreadedSearch(Position position, int allowedTimeMs, unsigned int threads = 1, int maxSearchDepth = MAX_DEPTH);
uint64_t BenchSearch(Position position, int maxSearchDepth = MAX_DEPTH);

int TexelSearch(Position& position, SearchData& data);

