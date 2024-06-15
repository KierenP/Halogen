#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "BitBoardDefine.h"
#include "Score.h"

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 64;

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
    Score Eval(Players stm) const;

    // call and then update inputs as required
    void AccumulatorPush();

    void AddInput(Square square, Pieces piece);
    void RemoveInput(Square square, Pieces piece);

    // do undo the last move
    void AccumulatorPop();

    static void Init();

private:
    static int index(Square square, Pieces piece, Players view);

    std::vector<HalfAccumulator> AccumulatorStack;

    alignas(32) static std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> hiddenWeights;
    alignas(32) static std::array<int16_t, HIDDEN_NEURONS> hiddenBias;
    alignas(32) static std::array<int16_t, HIDDEN_NEURONS * 2> outputWeights;
    alignas(32) static int16_t outputBias;
};
