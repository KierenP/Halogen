#pragma once
#include <cstdint>

#include "Move.h"
#include "bitboard.h"
#include "utility/static_vector.h"

// For move ordering we need to bundle the 'score' with the move objects
struct ExtendedMove
{
    ExtendedMove() = default;
    ExtendedMove(const Move _move)
        : move(_move)
        , score(0)
    {
    }
    ExtendedMove(Square from, Square to, MoveFlag flag)
        : move(from, to, flag)
        , score(0)
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
    int16_t score;
};

using ExtendedMoveList = StaticVector<ExtendedMove, 218>;
using BasicMoveList = StaticVector<Move, 218>;
