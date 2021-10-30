#pragma once
#include "BitBoardDefine.h"
#include "EvalCache.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 512;

class Position;

class Network
{
public:
    void Recalculate(const Position& position);

    // calculates starting from the first hidden layer and skips input -> hidden
    int16_t Eval() const;

    // call and then update inputs as required
    void AccumulatorPush();

    void AddInput(Square square, Pieces piece);
    void RemoveInput(Square square, Pieces piece);

    // do undo the last move
    void AccumulatorPop();

    static void Init();

private:
    std::vector<std::array<int16_t, HIDDEN_NEURONS>> Zeta;

    static std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> hiddenWeights;
    static std::array<int16_t, HIDDEN_NEURONS> hiddenBias;
    static std::array<int16_t, HIDDEN_NEURONS> outputWeights;
    static int16_t outputBias;
};
