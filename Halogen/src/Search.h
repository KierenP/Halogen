#pragma once

#include "SearchData.h"
#include "MoveGeneration.h"
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

void MultithreadedSearch(const Position& position, unsigned int threadCount, const SearchLimits& limits);
uint64_t BenchSearch(const Position& position, int maxSearchDepth = MAX_DEPTH);
