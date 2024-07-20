#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Score.h"

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 512;

constexpr size_t OUTPUT_BUCKETS = 8;

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
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
    alignas(64) std::array<std::array<int16_t, HIDDEN_NEURONS>, N_PLAYERS> side = {};

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
    std::array<AccumulatorTableEntry, KING_BUCKET_COUNT> king_bucket = {};

    void Recalculate(Accumulator& acc, const BoardState& board, Players side, Square king_sq);
};

class Network
{
public:
    Network();

    void Recalculate(const BoardState& board);

    // return true if the incrementally updated accumulators are correct
    bool Verify(const BoardState& board) const;

    // calculates starting from the first hidden layer and skips input -> hidden
    Score Eval(const BoardState& board) const;

    void ApplyMove(const BoardState& prev_move_board, const BoardState& post_move_board, Move move);
    void UndoMove();

private:
    std::vector<Accumulator> AccumulatorStack;
    AccumulatorTable table;
};
