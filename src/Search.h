#pragma once

#include "GameState.h"
#include "Move.h"
#include "Score.h"

class SearchSharedState;

/*Tuneable search constants*/

constexpr double LMR_constant = 1.05;
constexpr double LMR_depth_coeff = -2.06;
constexpr double LMR_move_coeff = 0.20;
constexpr double LMR_depth_move_coeff = 1.17;

constexpr int Null_constant = 4;
constexpr int Null_depth_quotent = 6;
constexpr int Null_beta_quotent = 250;

constexpr int Futility_constant = 20;
constexpr int Futility_coeff = 82;
constexpr int Futility_depth = 15;

constexpr Score aspiration_window_mid_width = 15;

constexpr int Delta_margin = 200;

constexpr int SNMP_coeff = 119;
constexpr int SNMP_depth = 8;

constexpr int LMP_constant = 11;
constexpr int LMP_coeff = 7;
constexpr int LMP_depth = 6;

/*----------------*/

struct SearchResult
{
    SearchResult(int score, Move move = Move::Uninitialized)
        : m_score(score)
        , m_move(move)
    {
    }

    SearchResult(Score score, Move move = Move::Uninitialized)
        : m_score(score)
        , m_move(move)
    {
    }

    Score GetScore() const
    {
        return m_score;
    }

    Move GetMove() const
    {
        return m_move;
    }

private:
    Score m_score;
    Move m_move;
};

void SearchThread(GameState& position, SearchSharedState& sharedData);