#pragma once

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/arch.hpp"
#include "network/inputs/king_bucket.h"
#include "network/inputs/threat.h"
#include "search/score.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Move;

namespace NN
{

// represents a pair of inputs (one for each side's king)
struct InputPair
{
    Square w_king;
    Square b_king;
    Square piece_sq;
    Piece piece;
};

// The main accumulator, composed of independently-updatable sub-accumulators for each input type.
// king_bucket stores bias + king-bucketed + piece-square combined; threats is updated separately.
struct Accumulator
{
    KingBucketPieceSquareAccumulator king_bucket;
    ThreatAccumulator threats;

    bool operator==(const Accumulator& rhs) const
    {
        return king_bucket == rhs.king_bucket && threats == rhs.threats;
    }

    void recalculate(const BoardState& board);
    void recalculate(const BoardState& board, Side side);

    // data for lazy updates
    bool acc_is_valid = false;
    bool white_requires_recalculation = false;
    bool black_requires_recalculation = false;
    std::array<InputPair, 2> adds = {};
    size_t n_adds = 0;
    std::array<InputPair, 2> subs = {};
    size_t n_subs = 0;
    BoardState board;
};

// An accumulator table entry caches the king-bucketed accumulator for a particular king position.
// When copying to the live accumulator, piece-square inputs are added on top from scratch.
struct AccumulatorTableEntry
{
    KingBucketPieceSquareAccumulator acc;
    std::array<uint64_t, N_PIECES> white_bb = {};
    std::array<uint64_t, N_PIECES> black_bb = {};
};

// A cache of king-bucketed accumulators for each king bucket. When we want to recalculate the
// king-bucketed accumulator, we use this as a base rather than initializing from scratch.
struct AccumulatorTable
{
    std::array<AccumulatorTableEntry, KING_BUCKET_COUNT * 2> king_bucket = {};

    void recalculate(Accumulator& acc, const BoardState& board, Side side, Square king_sq);
};

class Network
{
public:
    // called at the root of search
    void reset_new_search(const BoardState& board, Accumulator& acc);

    // return true if the incrementally updated accumulator is correct
    static bool verify(const BoardState& board, const Accumulator& acc);

    // calculates starting from the first hidden layer and skips input -> hidden
    static Score eval(const BoardState& board, const Accumulator& acc);

    // does a full from scratch recalculation
    static Score slow_eval(const BoardState& board);

    void store_lazy_updates(
        const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move);

    void apply_lazy_updates(const Accumulator& prev_acc, Accumulator& next_acc);

private:
    AccumulatorTable table;
};

}