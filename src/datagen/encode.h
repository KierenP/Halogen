#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "../BoardState.h"

struct training_data
{
    uint64_t occ;
    std::array<uint8_t, 16> pcs;
    int16_t score;
    int8_t result;
    uint8_t ksq;
    uint8_t opp_ksq;

    uint8_t padding[1];
    Move best_move;
};

static_assert(sizeof(training_data) == 32);
static_assert(std::is_standard_layout_v<training_data>);

/// - Score is stm relative, in Centipawns.
/// - Result is 0.0 for Black Win, 0.5 for Draw, 1.0 for White Win
training_data convert(BoardState board, int16_t score, float result, Move best_move);
