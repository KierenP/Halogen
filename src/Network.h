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

struct Accumulator
{
    alignas(64) std::array<std::array<int16_t, HIDDEN_NEURONS>, N_PLAYERS> side;

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

class Network
{
public:
    void Recalculate(const BoardState& board);

    // return true if the incrementally updated accumulators are correct
    bool Verify(const BoardState& board) const;

    // calculates starting from the first hidden layer and skips input -> hidden
    Score Eval(const BoardState& board) const;

    void ApplyMove(const BoardState& prev_move_board, const BoardState& post_move_board, Move move);
    void UndoMove();

private:
    std::vector<Accumulator> AccumulatorStack;
};
