#pragma once

#include "bitboard/enum.h"
#include "network/arch.hpp"
#include "network/inputs/piece_count.h"

#include <array>
#include <cstddef>
#include <cstdint>

class BoardState;
class Move;

namespace NN::PieceCount
{

// Piece-count accumulator update type, determined by store_lazy_updates based on the move flag.
enum class PieceCountDelta : uint8_t
{
    UNCHANGED, // quiet / castle / pawn double: counts and accum identical to prev
    COPY, // counts changed but no threshold crossed: accum side identical to prev
    SUB1, // one feature subtracted per perspective
    SUB2, // two features subtracted per perspective (rare)
    ADD1SUB1, // one add + one sub per perspective (promotion)
    ADD1SUB2, // one add + two subs per perspective (promotion capture)
};

// Piece-count accumulator. Stores the accumulated FT contributions from piece-count threshold features.
// Each perspective (WHITE/BLACK) has its own accumulator side, which sums the weights of active features.
struct PieceCountAccumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool acc_is_valid = false;

    // Lazy update delta info, set by store_lazy_updates
    PieceCountDelta delta = PieceCountDelta::UNCHANGED;
    std::array<uint8_t, N_SIDES> add_feature = {}; // per-perspective feature index to add
    std::array<std::array<uint8_t, 2>, N_SIDES> sub_feature = {}; // per-perspective feature indices to sub

    bool operator==(const PieceCountAccumulator& rhs) const;

    // Recalculate from scratch for both perspectives
    void recalculate_from_scratch(const BoardState& board, const NN::network& net);

    // Store lazy updates based on move flags — determine which piece counts changed
    void store_lazy_updates(const BoardState& prev_board, const BoardState& post_board, Move move);

    // Apply stored lazy updates from previous accumulator
    void apply_lazy_updates(const PieceCountAccumulator& prev, const NN::network& net);
};

} // namespace NN::PieceCount
