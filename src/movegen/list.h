#pragma once

#include "bitboard/define.h"
#include "movegen/move.h"
#include "utility/static_vector.h"

#include <algorithm>
#include <cstdint>

// For move ordering we need to bundle the 'score' with the move objects. The score is stored as an int16_t so the
// whole struct packs into 4 bytes; scores are clamped to that range, which only reorders moves in the extreme
// high-history tail.
struct ExtendedMove
{
    ExtendedMove() = default;
    constexpr ExtendedMove(const Move _move, int _score)
        : move(_move)
        , score(static_cast<int16_t>(std::clamp(_score, int { INT16_MIN }, int { INT16_MAX })))
    {
    }

    constexpr bool operator<(const ExtendedMove& rhs) const
    {
        return score < rhs.score;
    };

    constexpr bool operator>(const ExtendedMove& rhs) const
    {
        return score > rhs.score;
    };

    Move move;
    int16_t score;
};

static_assert(sizeof(ExtendedMove) == 4);

using ExtendedMoveList = StaticVector<ExtendedMove, MAX_LEGAL_MOVES>;
using BasicMoveList = StaticVector<Move, MAX_LEGAL_MOVES>;
