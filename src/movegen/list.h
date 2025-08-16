#pragma once
#include <cstdint>

#include "bitboard/define.h"
#include "movegen/move.h"
#include "utility/static_vector.h"

// For move ordering we need to bundle the 'score' with the move objects
struct ExtendedMove
{
    ExtendedMove() = default;
    ExtendedMove(const Move _move, int _score)
        : move(_move)
        , score(_score)
    {
    }

    bool operator<(const ExtendedMove& rhs) const
    {
        return score < rhs.score;
    };

    bool operator>(const ExtendedMove& rhs) const
    {
        return score > rhs.score;
    };

    Move move;
    int score;
};

using ExtendedMoveList = StaticVector<ExtendedMove, MAX_LEGAL_MOVES>;
using BasicMoveList = StaticVector<Move, MAX_LEGAL_MOVES>;
