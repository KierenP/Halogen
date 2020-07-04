#pragma once

#include "TranspositionTable.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Move.h"
#include "TimeManage.h"
#include <ctime>
#include <algorithm>
#include <utility>
#include <deque>
#include <xmmintrin.h>
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

struct ThreadData
{
	ThreadData();

	std::vector<std::vector<Move>> PvTable;
	std::vector<Killer> KillerMoves;							//2 moves indexed by distanceFromRoot
	unsigned int HistoryMatrix[N_PLAYERS][N_SQUARES][N_SQUARES];			//first index is from square and 2nd index is to square
	SearchTimeManage timeManage;
};

extern TranspositionTable tTable;
extern unsigned int ThreadCount;

Move MultithreadedSearch(Position position, int allowedTimeMs, unsigned int threads = 1, int maxSearchDepth = MAX_DEPTH);
uint64_t BenchSearch(Position position, int maxSearchDepth = MAX_DEPTH);
