#pragma once

#include <cstdint>

#include "GameState.h"
#include "Move.h"

class ThreadSharedData;
/*Tuneable search constants*/

constexpr double LMR_constant = -1.76;
constexpr double LMR_coeff = 1.03;

constexpr int Null_constant = 4;
constexpr int Null_depth_quotent = 6;
constexpr int Null_beta_quotent = 250;

constexpr int Futility_constant = 20;
constexpr int Futility_coeff = 82;
constexpr int Futility_depth = 15;

constexpr int Aspiration_window = 15;

constexpr int Delta_margin = 200;

constexpr int SNMP_coeff = 119;
constexpr int SNMP_depth = 8;

constexpr int LMP_constant = 10;
constexpr int LMP_coeff = 7;
constexpr int LMP_depth = 6;

/*----------------*/

struct SearchResult
{
    SearchResult(short score, Move move = Move::Uninitialized)
        : m_score(score)
        , m_move(move)
    {
    }

    int GetScore() const { return m_score; }
    Move GetMove() const { return m_move; }

private:
    short m_score;
    Move m_move;
};

uint64_t SearchThread(GameState position, ThreadSharedData& sharedData);