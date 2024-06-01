#pragma once

#include "GameState.h"
#include "Move.h"
#include "Score.h"

class SearchSharedState;

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