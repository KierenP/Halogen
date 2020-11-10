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
constexpr size_t HIDDEN_NEURONS = 128;

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

template<size_t INPUT_COUNT>
struct Neuron
{
    Neuron();
    Neuron(std::vector<int16_t> Weight, int16_t Bias);
    int32_t FeedForward(std::array<int16_t, INPUT_COUNT>& input) const;

    std::array<int16_t, INPUT_COUNT> weights;
    int16_t bias;
};

template<size_t INPUT_COUNT, size_t OUTPUT_COUNT>
struct HiddenLayer
{
    HiddenLayer(std::vector<int16_t> inputs);                                                                   // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::array<int16_t, OUTPUT_COUNT> FeedForward(std::array<int16_t, INPUT_COUNT>& input);

    void ApplyDelta(deltaArray& deltaVec);                                                         //incrementally update the connections between input layer and first hidden layer

    std::array<Neuron<INPUT_COUNT>, OUTPUT_COUNT>* neurons;
    std::array<int16_t, OUTPUT_COUNT> zeta;

private:
    std::array<int16_t, INPUT_COUNT * OUTPUT_COUNT>* weightTranspose;                                                                       //first neuron first weight, second neuron first weight etc...
};

struct Network
{
    Network(const std::vector<std::vector<int16_t>>& inputs);
    void RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs);

    void ApplyDelta(deltaArray& delta);                                                            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta();                                                                                   //for un-make moves
    int16_t QuickEval();                                                                                        //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:

    //hard code the number of layers here
    HiddenLayer <INPUT_NEURONS, HIDDEN_NEURONS> hiddenLayer;
    Neuron<HIDDEN_NEURONS> outputNeuron;

    std::array<std::array<int16_t, HIDDEN_NEURONS>, MAX_DEPTH>  OldZeta;
    size_t incrementalDepth = 0;
};

Network InitNetwork();