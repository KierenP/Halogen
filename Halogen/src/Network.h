#pragma once
#include "weights.h"
#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <random>
#include <numeric>
#include <algorithm>

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
    Neuron(std::vector<float> Weight, float Bias);
    float FeedForward(std::vector<float>& input) const;
    void Backpropogate(float delta_l, const std::vector<float>& prev_weights, float learnRate);
    void WriteToFile(std::ofstream& myfile);

    std::vector<float> weights;
    float bias;

    std::vector<float> grad;       //for adagrad
};

struct HiddenLayer
{
    HiddenLayer(std::vector<float> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<float> FeedForward(std::vector<float>& input);
    void Backpropogate(const std::vector<float>& delta_l, const std::vector<float>& prev_weights, float learnRate);
    void WriteToFile(std::ofstream& myfile);

    std::vector<Neuron> neurons;

    //cache for backprop after feedforward
    std::vector<float> zeta;    //weighted input

    void activation(const std::vector<float>& in, std::vector<float>& out);      
    std::vector<float> activationPrime(std::vector<float> x);

    void ApplyDelta(std::vector<deltaPoint>& delta, float forward);            //incrementally update the connections between input layer and first hidden layer

private:

    std::vector<float> weightTranspose; //first neuron first weight, second neuron first weight etc...
};

struct Network
{
    Network(std::vector<std::vector<float>> inputs, std::vector<size_t> hiddenNeuronCount);
    float FeedForward(std::vector<float> inputs);
    float Backpropagate(trainingPoint data, float learnRate);
    void WriteToFile();
    void Learn();

    void ApplyDelta(std::vector<deltaPoint>& delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta(std::vector<deltaPoint>& delta);     //for un-make moves
    float QuickEval();                                                         //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:
    std::vector<trainingPoint> quietlabeledDataset();
    std::vector<trainingPoint> Stockfish3PerDataset();
    size_t inputNeurons;

    void AddExtraNullLayer(size_t neurons);   //given a network add another hidden layer at the end that wont change network output.

    //cache for backprop after feedforward (these are for the output neuron)
    float zeta;    //weighted input
    float alpha;   //result after activation function

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;
};

void Learn();

Network InitNetwork(std::string file);
Network InitNetwork();
Network CreateRandom(std::vector<size_t> hiddenNeuronCount);