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

struct Network
{
    Network(std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>* HiddenWeights,
            std::array<int16_t, HIDDEN_NEURONS>* HiddenBias,
            std::array<int16_t, HIDDEN_NEURONS>* OutputWeights,
            int16_t* OutputBias);

    void RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs);

    void ApplyDelta(deltaArray& delta);                                                            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta();                                                                                   //for un-make moves
    int16_t QuickEval();                                                                                        //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:

    //hard architecture here
    std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> hiddenWeights;
    std::array<int16_t, HIDDEN_NEURONS> hiddenBias;
    std::array<int16_t, HIDDEN_NEURONS> outputWeights;
    int16_t outputBias;

    std::array<std::array<int32_t, HIDDEN_NEURONS>, MAX_DEPTH>  Zeta;
    size_t incrementalDepth = 0;
};

Network* InitNetwork();