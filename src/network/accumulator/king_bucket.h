#pragma once

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/arch.hpp"
#include "network/inputs/king_bucket.h"

#include <array>
#include <cstddef>
#include <cstdint>

class Move;

namespace NN::KingBucket
{

struct AccumulatorTable;

// Accumulator for king-bucketed piece-square inputs.
// The 'side' array stores: bias + king-bucketed weights combined.
struct KingBucketAccumulator
{
    // represents a pair of inputs (one for each side)
    struct InputPair
    {
        Square w_king;
        Square b_king;
        Square piece_sq;
        Piece piece;
    };

    // represents a single input on one accumulator side
    struct Input
    {
        Square king_sq;
        Square piece_sq;
        Piece piece;
    };

    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const KingBucketAccumulator& rhs) const;

    void add_input(const Input& input, Side view, const NN::network& net);
    void sub_input(const Input& input, Side view, const NN::network& net);

    void add1sub1(
        const KingBucketAccumulator& prev, const Input& a1, const Input& s1, Side view, const NN::network& net);
    void add1sub1(
        const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, Side view, const NN::network& net);
    void add1sub1(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const NN::network& net);

    void add1sub2(const KingBucketAccumulator& prev, const Input& a1, const Input& s1, const Input& s2, Side view,
        const NN::network& net);
    void add1sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const InputPair& s2,
        const NN::network& net);
    void add1sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const InputPair& s2,
        Side view, const NN::network& net);

    void add2sub2(const KingBucketAccumulator& prev, const Input& a1, const Input& a2, const Input& s1, const Input& s2,
        Side view, const NN::network& net);
    void add2sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& a2, const InputPair& s1,
        const InputPair& s2, Side view, const NN::network& net);
    void add2sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& a2, const InputPair& s1,
        const InputPair& s2, const NN::network& net);

    void recalculate_from_scratch(const BoardState& board_, const NN::network& net);
    void store_lazy_updates(const BoardState& prev_move_board, const BoardState& post_move_board, Move move);
    void apply_lazy_updates(const KingBucketAccumulator& prev_acc, AccumulatorTable& table, const NN::network& net);

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
    KingBucketAccumulator acc;
    std::array<uint64_t, N_PIECES> white_bb = {};
    std::array<uint64_t, N_PIECES> black_bb = {};
};

// A cache of king-bucketed accumulators for each king bucket. When we want to recalculate the
// king-bucketed accumulator, we use this as a base rather than initializing from scratch.
struct AccumulatorTable
{
    std::array<AccumulatorTableEntry, KING_BUCKET_COUNT * 2> king_bucket = {};

    void recalculate(
        KingBucketAccumulator& acc, const BoardState& board, Side side, Square king_sq, const NN::network& net);

    void reset_table(const std::array<int16_t, NN::FT_SIZE>& ft_bias);
};
}