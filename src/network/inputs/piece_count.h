#pragma once

#include "bitboard/enum.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace NN::PieceCount
{

// Piece-count threshold features (15 per side, 30 total):
//   queen>=1 (1) + rook>=2,>=1 (2) + bishop>=2,>=1 (2) + knight>=2,>=1 (2) + pawn>=8..>=1 (8) = 15
//
// Layout per side:
//   [0]     queen >= 1
//   [1]     rook >= 2
//   [2]     rook >= 1
//   [3]     bishop >= 2
//   [4]     bishop >= 1
//   [5]     knight >= 2
//   [6]     knight >= 1
//   [7-14]  pawn >= 8 down to pawn >= 1
//
// Features [0..15) = "my" pieces, [15..30) = "opponent's" pieces (from each perspective).

constexpr size_t PIECE_COUNT_PER_SIDE = 15;
constexpr size_t PIECE_COUNT_TOTAL = PIECE_COUNT_PER_SIDE * 2;

// Maximum number of active piece-count features per perspective (all thresholds met for both sides)
constexpr size_t MAX_ACTIVE_PIECE_COUNT = PIECE_COUNT_TOTAL;

// Given piece counts for one side, emit the active feature indices relative to the side's base offset.
// Returns the number of active features.
template <typename Callback>
constexpr size_t emit_features_for_side(const std::array<uint8_t, N_PIECE_TYPES>& counts, size_t base, Callback&& cb)
{
    size_t n = 0;

    // Queen >= 1
    if (counts[QUEEN] >= 1)
    {
        cb(base + 0);
        n++;
    }
    // Rook >= 2
    if (counts[ROOK] >= 2)
    {
        cb(base + 1);
        n++;
    }
    // Rook >= 1
    if (counts[ROOK] >= 1)
    {
        cb(base + 2);
        n++;
    }
    // Bishop >= 2
    if (counts[BISHOP] >= 2)
    {
        cb(base + 3);
        n++;
    }
    // Bishop >= 1
    if (counts[BISHOP] >= 1)
    {
        cb(base + 4);
        n++;
    }
    // Knight >= 2
    if (counts[KNIGHT] >= 2)
    {
        cb(base + 5);
        n++;
    }
    // Knight >= 1
    if (counts[KNIGHT] >= 1)
    {
        cb(base + 6);
        n++;
    }
    // Pawn >= 8 down to >= 1
    for (uint8_t k = 8; k >= 1; k--)
    {
        if (counts[PAWN] >= k)
        {
            cb(base + 7 + (8 - k));
            n++;
        }
    }

    return n;
}

// Compute piece counts for each side from a board state.
// counts[WHITE] and counts[BLACK] are indexed by PieceType.
template <typename BoardState>
constexpr std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES> compute_piece_counts(const BoardState& board)
{
    std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES> counts {};

    for (int pt = PAWN; pt < KING; pt++) // skip KING — king is always present
    {
        counts[WHITE][pt]
            = static_cast<uint8_t>(std::popcount(board.get_pieces_bb(get_piece(static_cast<PieceType>(pt), WHITE))));
        counts[BLACK][pt]
            = static_cast<uint8_t>(std::popcount(board.get_pieces_bb(get_piece(static_cast<PieceType>(pt), BLACK))));
    }

    return counts;
}

// Emit all active piece-count feature indices for a given perspective.
// "my" features [0..15), "opponent's" features [15..30)
template <typename BoardState, typename Callback>
constexpr void emit_all_features(const BoardState& board, Side perspective, Callback&& cb)
{
    auto counts = compute_piece_counts(board);

    // "my" pieces = perspective's pieces → base 0
    // "opponent's" pieces = !perspective's pieces → base PIECE_COUNT_PER_SIDE
    emit_features_for_side(counts[perspective], 0, cb);
    emit_features_for_side(counts[!perspective], PIECE_COUNT_PER_SIDE, cb);
}

} // namespace NN::PieceCount
