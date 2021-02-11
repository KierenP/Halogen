#pragma once

#include "Pyrrhic/tbprobe.h"
#include "MoveGenerator.h"
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

extern int SNMP_depth;
extern int SNMP_coeff;

/*----------------*/

struct SearchResult
{
	SearchResult(short score, Move move = Move()) : m_score(score), m_move(move) {}
	~SearchResult() {}

	int GetScore() const { return m_score; }
	Move GetMove() const { return m_move; }

private:
	short m_score;
	Move m_move;
};

uint64_t SearchThread(const Position& position, const SearchParameters& parameters, const SearchLimits& limits, bool noOutput = false);