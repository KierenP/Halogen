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

#define INPUT_NEURONS 12 * 64 - 32 + 1 + 4

struct trainingPoint
{
    trainingPoint(std::array<bool, INPUT_NEURONS> input, float gameResult);

    std::array<bool, INPUT_NEURONS> inputs;
    float result;
};

struct deltaPoint
{
    size_t index;
    int delta;
};

struct Neuron
{
    Neuron(const std::vector<float>& Weight, float Bias);
    float FeedForward(std::vector<float>& input, bool UseReLU) const;

    std::vector<float> weights;
    float bias;
};

struct HiddenLayer
{
    HiddenLayer(std::vector<float> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<float> FeedForward(std::vector<float>& input, bool UseReLU);

    std::vector<Neuron> neurons;
    void ApplyDelta(std::vector<deltaPoint>& deltaVec);            //incrementally update the connections between input layer and first hidden layer

    std::vector<float> zeta;

private:
    std::vector<float> weightTranspose; //first neuron first weight, second neuron first weight etc...
};

struct Network
{
    Network(std::vector<std::vector<float>> inputs, std::vector<size_t> NeuronCount);
    float FeedForward(std::vector<float> inputs);

    void ApplyDelta(std::vector<deltaPoint> delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta();     //for un-make moves
    float QuickEval();                                                         //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:
    size_t inputNeurons;

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;

    float zeta;

    std::vector<std::vector<float>> OldZeta;
    size_t incrementalDepth = 0;
};

Network InitNetwork();