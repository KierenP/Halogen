#pragma once
#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <random>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <cstring>
#include "EvalCache.h"
#include "BitBoardDefine.h"

constexpr size_t INPUT_NEURONS = 12 * 64;
constexpr size_t HIDDEN_NEURONS = 256;

constexpr int16_t MAX_VALUE = 128;
constexpr int16_t PRECISION = ((size_t)std::numeric_limits<int16_t>::max() + 1) / MAX_VALUE;
constexpr int32_t SQUARE_PRECISION = (int32_t)PRECISION * PRECISION;
constexpr int32_t HALF_SQUARE_PRECISION = SQUARE_PRECISION / 2;
constexpr int16_t HALF_PRECISION = PRECISION / 2;

struct deltaArray
{
    struct deltaPoint
    {
        size_t index;
        int16_t delta;
    };

    size_t size;
    deltaPoint deltas[4];
};

extern std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>* hiddenWeights;
extern std::array<int16_t, HIDDEN_NEURONS>* hiddenBias;
extern std::array<int16_t, HIDDEN_NEURONS>* outputWeights;
extern int16_t* outputBias;

void RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs, std::vector<std::array<int16_t, HIDDEN_NEURONS>>& Zeta);
void ApplyDelta(const deltaArray& update, std::vector<std::array<int16_t, HIDDEN_NEURONS>>& Zeta);  //incrementally update the connections between input layer and first hidden layer
void ApplyInverseDelta(std::vector<std::array<int16_t, HIDDEN_NEURONS>>& Zeta);                     //for un-make moves
int16_t QuickEval(const std::vector<std::array<int16_t, HIDDEN_NEURONS>>& Zeta);                    //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

void NetworkInit();