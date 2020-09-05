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
    trainingPoint(std::array<bool, INPUT_NEURONS> input, double gameResult);

    std::array<bool, INPUT_NEURONS> inputs;
    double result;
};

struct Neuron
{
    Neuron(std::vector<double> Weight, double Bias);
    double FeedForward(std::vector<double>& input) const;
    void Backpropogate(double delta_l, const std::vector<double>& prev_weights, double learnRate);
    void WriteToFile(std::ofstream& myfile);

    std::vector<double> weights;
    double bias;

    std::vector<double> grad;       //for adagrad
};

struct HiddenLayer
{
    HiddenLayer(std::vector<double> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<double> FeedForward(std::vector<double>& input);
    void Backpropogate(const std::vector<double>& delta_l, const std::vector<double>& prev_weights, double learnRate);
    void WriteToFile(std::ofstream& myfile);

    std::vector<Neuron> neurons;

    //cache for backprop after feedforward
    std::vector<double> zeta;    //weighted input
    std::vector<double> alpha;   //result after activation function

    void activation(std::vector<double>& in, std::vector<double>& out);
    std::vector<double> activationPrime(std::vector<double> x);

    void ApplyDelta(std::vector<std::pair<size_t, double>>& delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta(std::vector<std::pair<size_t, double>>& delta);     //for un-make moves
};

struct Network
{
    Network(std::vector<std::vector<double>> inputs, std::vector<size_t> hiddenNeuronCount);
    double FeedForward(std::vector<double> inputs);
    double Backpropagate(trainingPoint data, double learnRate);
    void WriteToFile();
    void Learn();

    void ApplyDelta(std::vector<std::pair<size_t, double>>& delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta(std::vector<std::pair<size_t, double>>& delta);     //for un-make moves
    double QuickEval();                                                         //when used with above, this just calculates starting from the alpha of first hidden layer and skips input -> hidden

private:
    std::vector<trainingPoint> quietlabeledDataset();
    std::vector<trainingPoint> Stockfish3PerDataset();
    size_t inputNeurons;

    //cache for backprop after feedforward (these are for the output neuron)
    double zeta;    //weighted input
    double alpha;   //result after activation function

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;
};

void Learn();

Network InitNetwork(std::string file);
Network InitNetwork();
Network CreateRandom(std::vector<size_t> hiddenNeuronCount);