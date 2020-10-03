#include "Network.h"

static const char* WeightsTXT[] = {
    #include "741_128_1-200.network" 
    ""
};

Network InitNetwork()
{
    std::stringstream stream;

    for (int i = 0; strcmp(WeightsTXT[i], ""); i++)
        stream << WeightsTXT[i] << "\n";

    std::string line;

    std::vector<std::vector<float>> weights;
    std::vector<size_t> LayerNeurons;
    weights.push_back({});

    while (getline(stream, line))
    {
        std::istringstream iss(line);
        std::string token;

        iss >> token;

        if (token == "InputNeurons")
        {
            iss >> token;
            LayerNeurons.push_back(stoull(token));
        }

        if (token == "HiddenLayerNeurons")
        {
            std::vector<float> layerWeights;

            iss >> token;
            size_t num = stoull(token);

            LayerNeurons.push_back(num);

            for (size_t i = 0; i < num; i++)
            {
                getline(stream, line);
                std::istringstream lineStream(line);

                while (lineStream >> token)
                {
                    layerWeights.push_back(stof(token));
                }
            }

            weights.push_back(layerWeights);
        }

        else if (token == "OutputLayer")
        {
            LayerNeurons.push_back(1);  //always 1 output neuron
            std::vector<float> layerWeights;
            getline(stream, line);
            std::istringstream lineStream(line);
            while (lineStream >> token)
            {
                layerWeights.push_back(stof(token));
            }
            weights.push_back(layerWeights);
        }
    }

    return Network(weights, LayerNeurons);
}

Neuron::Neuron(const std::vector<float>& Weight, float Bias) : weights(Weight)
{
    bias = Bias;
}

float Neuron::FeedForward(std::vector<float>& input, bool UseReLU) const
{
    assert(input.size() == weights.size());

    float ret = bias;

    for (size_t i = 0; i < input.size(); i++)
    {
        if (UseReLU)
            ret += std::max(0.f, input[i]) * weights[i];
        else
            ret += input[i] * weights[i];
    }

    return ret;
}

HiddenLayer::HiddenLayer(std::vector<float> inputs, size_t NeuronCount)
{
    assert(inputs.size() % NeuronCount == 0);

    size_t WeightsPerNeuron = inputs.size() / NeuronCount;

    for (size_t i = 0; i < NeuronCount; i++)
    {
        neurons.push_back(Neuron(std::vector<float>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1)));
    }

    for (size_t i = 0; i < WeightsPerNeuron - 1; i++)
    {
        for (size_t j = 0; j < NeuronCount; j++)
        {
            weightTranspose.push_back(neurons.at(j).weights.at(i));
        }
    }

    zeta = std::vector<float>(NeuronCount, 0);
}

std::vector<float> HiddenLayer::FeedForward(std::vector<float>& input, bool UseReLU)
{
    OldZeta.clear();

    for (size_t i = 0; i < neurons.size(); i++)
    {
        zeta[i] = neurons.at(i).FeedForward(input, UseReLU);
    }

    return zeta;
}

void HiddenLayer::ApplyDelta(std::vector<deltaPoint>& deltaVec)
{
    OldZeta.push_back(zeta);

    size_t neuronCount = zeta.size();
    size_t deltaCount = deltaVec.size();

    for (size_t index = 0; index < deltaCount; index++)
    {
        float deltaValue = deltaVec[index].delta;
        size_t weightTransposeIndex = deltaVec[index].index * neuronCount;

        for (size_t neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += weightTranspose[weightTransposeIndex + neuron] * deltaValue;
        }
    }
}

void HiddenLayer::ReverseDelta()
{
    zeta = OldZeta.back();
    OldZeta.pop_back();
}

Network::Network(std::vector<std::vector<float>> inputs, std::vector<size_t> NeuronCount) : outputNeuron(std::vector<float>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    assert(inputs.size() == NeuronCount.size());

    zeta = 0;
    inputNeurons = NeuronCount.at(0);

    for (size_t i = 1; i < NeuronCount.size() - 1; i++)
    {
        hiddenLayers.push_back(HiddenLayer(inputs.at(i), NeuronCount.at(i)));
    }
}

float Network::FeedForward(std::vector<float> inputs)
{
    assert(inputs.size() == inputNeurons);

    for (size_t i = 0; i < hiddenLayers.size(); i++)
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs, i != 0);
    }

    zeta = outputNeuron.FeedForward(inputs, true);

    return zeta;
}

void Network::ApplyDelta(std::vector<deltaPoint> delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta);
}

void Network::ApplyInverseDelta()
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ReverseDelta();
}

float Network::QuickEval()
{
    std::vector<float>& inputs = hiddenLayers.at(0).zeta;

    for (size_t i = 1; i < hiddenLayers.size(); i++)    //skip first layer
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs, true);
    }

    zeta = outputNeuron.FeedForward(inputs, true);

    return zeta;
}

trainingPoint::trainingPoint(std::array<bool, INPUT_NEURONS> input, float gameResult) : inputs(input)
{
    result = gameResult;
}