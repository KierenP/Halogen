#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Score.h"

class Move;

constexpr size_t INPUT_SIZE = 12 * 64;
constexpr size_t FT_SIZE = 1024;
constexpr size_t L1_SIZE = 16;
constexpr size_t L2_SIZE = 32;

constexpr size_t OUTPUT_BUCKETS = 8;

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
};
// clang-format on

constexpr size_t KING_BUCKET_COUNT = []()
{
    auto [min, max] = std::minmax_element(KING_BUCKETS.begin(), KING_BUCKETS.end());
    return *max - *min + 1;
}();

// represents a single input on one accumulator side
struct Input
{
    Square king_sq;
    Square piece_sq;
    Pieces piece;
};

// represents a pair of inputs (one on each accumulator side)
struct InputPair
{
    Square w_king;
    Square b_king;
    Square piece_sq;
    Pieces piece;
};

// stored the accumulated first layer values for each side
struct Accumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_PLAYERS> side = {};

    bool operator==(const Accumulator& rhs) const
    {
        return side == rhs.side;
    }

    void AddInput(const InputPair& input);
    void AddInput(const Input& input, Players side);

    void SubInput(const InputPair& input);
    void SubInput(const Input& input, Players side);

    void Recalculate(const BoardState& board);
    void Recalculate(const BoardState& board, Players side);

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

// An accumulator, along with the bitboards that resulted in the white/black accumulated values. Note that the board
// cached in each side might be different, so white_bb != black_bb
struct AccumulatorTableEntry
{
    Accumulator acc;
    std::array<uint64_t, N_PIECES> white_bb = {};
    std::array<uint64_t, N_PIECES> black_bb = {};
};

// A cache of accumulators for each king bucket. When we want to recalculate the accumulator, we use this as a base
// rather than initializing from scratch
struct AccumulatorTable
{
    std::array<AccumulatorTableEntry, KING_BUCKET_COUNT * 2> king_bucket = {};

    void Recalculate(Accumulator& acc, const BoardState& board, Players side, Square king_sq);
};

class Network
{
public:
    // called at the root of search
    void Reset(const BoardState& board, Accumulator& acc);

    // return true if the incrementally updated accumulator is correct
    static bool Verify(const BoardState& board, const Accumulator& acc);

    // calculates starting from the first hidden layer and skips input -> hidden
    static Score Eval(const BoardState& board, const Accumulator& acc);

    // does a full from scratch recalculation
    static Score SlowEval(const BoardState& board);

    void StoreLazyUpdates(
        const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move);

    void ApplyLazyUpdates(const Accumulator& prev_acc, Accumulator& next_acc);

private:
    AccumulatorTable table;
};
