#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "Score.h"

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 768;

constexpr size_t OUTPUT_BUCKETS = 8;

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 0, 1, 1, 1, 1, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 
};
// clang-format on

constexpr size_t KING_BUCKET_COUNT = []()
{
    auto [min, max] = std::minmax_element(KING_BUCKETS.begin(), KING_BUCKETS.end());
    return *max - *min + 1;
}();

class BoardState;

// represents a single input on one accumulator side
struct Input
{
    Square king_sq;
    Square piece_sq;
    Pieces piece;
};

// stored the accumulated first layer values for one side
struct Accumulator
{
    alignas(64) std::array<int16_t, HIDDEN_NEURONS> values = {};

    bool operator==(const Accumulator& rhs) const
    {
        return values == rhs.values;
    }

    bool operator!=(const Accumulator& rhs) const
    {
        return !(*this == rhs);
    }

    void AddInput(const Input& input, Players view);
    void SubInput(const Input& input, Players view);
    void Recalculate(const BoardState& board, Players view);

    // data for lazy updates
    bool acc_is_valid = false;
    bool requires_recalculation = false;
    std::array<Input, 2> adds = {};
    size_t n_adds = 0;
    std::array<Input, 2> subs = {};
    size_t n_subs = 0;
    BoardState board;
};

// An accumulator, along with the bitboards that resulted in the accumulated values.
struct AccumulatorTableEntry
{
    Accumulator acc;
    std::array<uint64_t, N_PIECES> bb = {};
};

// A cache of accumulators for each king bucket. When we want to recalculate the accumulator, we use this as a base
// rather than initializing from scratch
struct AccumulatorTable
{
    std::array<std::array<AccumulatorTableEntry, KING_BUCKET_COUNT * 2>, N_PLAYERS> side = {};

    void Recalculate(Accumulator& acc, const BoardState& board, Players side, Square king_sq);
};

class Network
{
public:
    // called at the root of search (for each side)
    void Reset(const BoardState& board, std::array<Accumulator, N_PLAYERS>& acc);

    // return true if the incrementally updated accumulator is correct
    static bool Verify(const BoardState& board, const std::array<Accumulator, N_PLAYERS>& acc);

    // calculates starting from the first hidden layer and skips input -> hidden
    static Score Eval(const BoardState& board, const std::array<Accumulator, N_PLAYERS>& acc);

    // does a full from scratch recalculation
    static Score SlowEval(const BoardState& board);

    void StoreLazyUpdates(const BoardState& prev_move_board, const BoardState& post_move_board,
        std::array<Accumulator, N_PLAYERS>& acc, Move move);

    void ApplyLazyUpdates(const Accumulator& prev_acc, Accumulator& next_acc, Players view);

private:
    AccumulatorTable table;
};
