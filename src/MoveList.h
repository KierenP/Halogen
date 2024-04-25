#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Score.h"
#include "StaticVector.h"

// For move ordering we need to bundle the 'score' and SEE values
// with the move objects
struct ExtendedMove
{
    ExtendedMove() = default;
    ExtendedMove(const Move _move)
        : move(_move)
        , score(0)
        , SEE(0)
    {
    }
    ExtendedMove(Square from, Square to, MoveFlag flag)
        : move(from, to, flag)
        , score(0)
        , SEE(0)
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
    int16_t SEE;
};

using ExtendedMoveList = StaticVector<ExtendedMove, 256>;
using BasicMoveList = StaticVector<Move, 256>;
