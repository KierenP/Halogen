#pragma once

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/arch.hpp"
#include "search/score.h"

#include <array>
#include <cstdint>
#include <cstring>

class Move;

namespace NN
{

// represents a single input on one accumulator side (king-bucketed or psqt)
struct Input
{
    Square king_sq;
    Square piece_sq;
    Piece piece;
};

// represents a pair of inputs (one on each accumulator side)
struct InputPair
{
    Square w_king;
    Square b_king;
    Square piece_sq;
    Piece piece;
};

// Stores the accumulated first layer values for each side. This contains the incrementally-updatable part of the
// feature transformer: king-bucketed piece-square features and unbucketed piece-square features. Threat features
// are computed at eval time and added on top.
struct Accumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const Accumulator& rhs) const
    {
        return side == rhs.side;
    }

    // King-bucketed input operations
    void add_king_input(const InputPair& input);
    void add_king_input(const Input& input, Side side);
    void sub_king_input(const InputPair& input);
    void sub_king_input(const Input& input, Side side);

    // Unbucketed piece-square input operations
    void add_psqt_input(const InputPair& input);
    void add_psqt_input(const Input& input, Side side);
    void sub_psqt_input(const InputPair& input);
    void sub_psqt_input(const Input& input, Side side);

    // Combined: add both king-bucketed and psqt inputs for a piece
    void add_input(const InputPair& input);
    void add_input(const Input& input, Side side);
    void sub_input(const InputPair& input);
    void sub_input(const Input& input, Side side);

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

    void recalculate(Accumulator& acc, const BoardState& board, Side side, Square king_sq);
};

// Compute threat features and add them to a copy of the accumulator for eval.
// This produces a temporary accumulator with threats applied on top.
void apply_threat_features(const BoardState& board, const std::array<int16_t, FT_SIZE>& base_stm,
    const std::array<int16_t, FT_SIZE>& base_nstm, std::array<int16_t, FT_SIZE>& out_stm,
    std::array<int16_t, FT_SIZE>& out_nstm);

// TODO: this class can mostly disappear and be replaced with stand alone functions
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