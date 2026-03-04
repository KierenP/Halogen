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

// Piece-count accumulator. Stores the accumulated FT contributions from piece-count threshold features.
// Each perspective (WHITE/BLACK) has its own accumulator side, which sums the weights of active features.
struct PieceCountAccumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    // Piece counts per side, indexed by [Side][PieceType] (excludes KING).
    std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES> counts = {};

    bool acc_is_valid = false;

    bool operator==(const PieceCountAccumulator& rhs) const;

    // Recalculate from scratch for both perspectives
    void recalculate_from_scratch(const BoardState& board, const NN::network& net);

    // Store lazy updates based on move flags — determine which piece counts changed
    void store_lazy_updates(const BoardState& prev_board, const BoardState& post_board, Move move);

    // Apply stored lazy updates from previous accumulator
    void apply_lazy_updates(const PieceCountAccumulator& prev, const NN::network& net);
};

} // namespace NN::PieceCount
