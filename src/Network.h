#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "BitBoardDefine.h"

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 8;

class Position;

struct HalfAccumulator
{
    std::array<float, HIDDEN_NEURONS> vals;
};

class Network
{
public:
    void Recalculate(const Position& position);

    // calculates starting from the first hidden layer and skips input -> hidden
    int16_t Eval(Players stm) const;

    // call and then update inputs as required
    void AccumulatorPush();

    void AddInput(Square square, Pieces piece);
    void RemoveInput(Square square, Pieces piece);

    // do undo the last move
    void AccumulatorPop();

    static void Init();

private:
    std::vector<HalfAccumulator> AccumulatorStack;

    static std::array<std::array<float, HIDDEN_NEURONS>, INPUT_NEURONS> hiddenWeights;
    static std::array<float, HIDDEN_NEURONS> hiddenBias;
    static std::array<float, HIDDEN_NEURONS> outputWeights;
    static float outputBias;
};
