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

constexpr size_t INPUT_NEURONS = 12 * 64;

constexpr int16_t MAX_VALUE = 128;
constexpr int16_t PRECISION = ((size_t)std::numeric_limits<int16_t>::max() + 1) / 128;
constexpr int16_t HALF_PRECISION = PRECISION / 2;

constexpr int16_t log2u16(int16_t n)
{
    return ((n < 2) ? 0 : 1 + log2u16(n / 2));
}

constexpr int16_t PRECISION_SHIFT = log2u16(PRECISION);

struct deltaPoint
{
    size_t index;
    int8_t delta;
};

struct Neuron
{
    Neuron(const std::vector<int16_t>& Weight, int16_t Bias);
    int32_t FeedForward(std::vector<int16_t>& input, bool UseReLU) const;

    std::vector<int16_t> weights;
    int16_t bias;
};

struct HiddenLayer
{
    HiddenLayer(std::vector<int16_t> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<int16_t> FeedForward(std::vector<int16_t>& input, bool UseReLU);

    std::vector<Neuron> neurons;
    void ApplyDelta(std::vector<deltaPoint>& deltaVec);            //incrementally update the connections between input layer and first hidden layer

    std::vector<int16_t> zeta;

private:
    std::vector<int16_t> weightTranspose; //first neuron first weight, second neuron first weight etc...
};

struct Network
{
    Network(std::vector<std::vector<int16_t>> inputs, std::vector<size_t> NeuronCount);
    void RecalculateIncremental(std::vector<int16_t> inputs);

    void ApplyDelta(std::vector<deltaPoint>& delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta();     //for un-make moves
    int16_t QuickEval();                                        //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:
    size_t inputNeurons;

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;

    std::vector<std::vector<int16_t>> OldZeta;
    size_t incrementalDepth = 0;
};

Network InitNetwork();