#pragma once

#include "Pyrrhic/tbprobe.h"
#include "MoveGenerator.h"
#include <ctime>
#include <algorithm>
#include <thread>
#include <cmath>

/*Tuneable search constants*/

inline double LMR_constant = -1.76;
inline double LMR_coeff = 1.03;

inline int Null_constant = 4;
inline int Null_depth_quotent = 6;
inline int Null_beta_quotent = 250;

inline int Futility_constant = 20;
inline int Futility_coeff = 82;
inline int Futility_depth = 15;

inline int Aspiration_window = 15;

inline int Delta_margin = 200;

inline int SNMP_coeff = 119;
inline int SNMP_depth = 8;

inline int LMP_constant = 10;
inline int LMP_coeff = 7;
inline int LMP_depth = 6;

/*----------------*/

struct SearchResult
{
	SearchResult(short score, Move move = Move::Uninitialized) : m_score(score), m_move(move) {}

	int GetScore() const { return m_score; }
	Move GetMove() const { return m_move; }

private:
	short m_score;
	Move m_move;
};

uint64_t SearchThread(Position position, SearchParameters parameters, const SearchLimits& limits, bool noOutput = false);

int PlayoutGame(Position position, int pieceCount);