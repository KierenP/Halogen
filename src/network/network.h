#pragma once

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/accumulator/king_bucket.h"
#include "network/accumulator/threat.h"
#include "network/arch.hpp"
#include "search/score.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Move;

namespace NN
{

// The main accumulator, composed of independently-updatable sub-accumulators for each input type.
// king_bucket stores bias + king-bucketed; threats is updated separately.
struct Accumulator
{
    KingBucket::KingBucketAccumulator king_bucket;
    Threats::ThreatAccumulator threats;

    bool operator==(const Accumulator& rhs) const
    {
        return king_bucket == rhs.king_bucket && threats == rhs.threats;
    }

    void recalculate(const BoardState& board);
    void recalculate(const BoardState& board, Side side);

    bool acc_is_valid = false;
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
    KingBucket::AccumulatorTable table;
};

}