#pragma once
#include "TranspositionTable.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Evaluate.h"
#include "Move.h"
#include "TimeManage.h"
#include <ctime>
#include <algorithm>
#include <utility>
#include <deque>
#include <xmmintrin.h>

struct SearchResult
{
	SearchResult(int score, Move move = Move()) { m_score = score; m_move = move; }
	~SearchResult() {}

	int GetScore() { return m_score; }
	Move GetMove() { return m_move; }

private:
	int m_score;
	Move m_move;
};

extern TranspositionTable tTable;
Move SearchPosition(Position position, int allowedTimeMs);
