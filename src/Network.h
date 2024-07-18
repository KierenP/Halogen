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

struct HalfAccumulator
{
    alignas(32) std::array<std::array<int16_t, HIDDEN_NEURONS>, N_PLAYERS> side;

    bool operator==(const HalfAccumulator& rhs) const
    {
        return side == rhs.side;
    }
};

class Network
{
public:
    void Recalculate(const BoardState& board);

    // return true if the incrementally updated accumulators are correct
    bool Verify(const BoardState& board) const;

    // calculates starting from the first hidden layer and skips input -> hidden
    Score Eval(const BoardState& board) const;

    // void ApplyMove(const BoardState& board, Move move);
    // void UndoMove(const BoardState& board);

private:
    std::vector<HalfAccumulator> AccumulatorStack;
};
