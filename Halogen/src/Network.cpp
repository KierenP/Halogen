#include "Network.h"

static const char* WeightsTXT[] = {
    #include "epoch1751_b8192_quant.nn"
    ""
};

Network InitNetwork()
{
    std::stringstream stream;

    for (int i = 0; strcmp(WeightsTXT[i], ""); i++)
        stream << WeightsTXT[i] << "\n";

    std::string line;

    std::vector<std::vector<int16_t>> weights;
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
            std::vector<int16_t> layerWeights;

            iss >> token;
            size_t num = stoull(token);

            LayerNeurons.push_back(num);

            for (size_t i = 0; i < num; i++)
            {
                getline(stream, line);
                std::istringstream lineStream(line);

                while (lineStream >> token)
                {
                    layerWeights.push_back(stoull(token));
                }
            }

            weights.push_back(layerWeights);
        }

        else if (token == "OutputLayer")
        {
            LayerNeurons.push_back(1);  //always 1 output neuron
            std::vector<int16_t> layerWeights;
            getline(stream, line);
            std::istringstream lineStream(line);
            while (lineStream >> token)
            {
                layerWeights.push_back(stoull(token));
            }
            weights.push_back(layerWeights);
        }
    }

    return Network(weights, LayerNeurons);
}

Neuron::Neuron(const std::vector<int16_t>& Weight, int16_t Bias) : weights(Weight)
{
    bias = Bias;
}

int32_t Neuron::FeedForward(std::vector<int16_t>& input, bool UseReLU) const
{
    assert(input.size() == weights.size());

    int32_t ret = bias;

    for (size_t i = 0; i < input.size(); i++)
    {
        if (UseReLU)
            ret += std::max(int16_t(0), input[i]) * weights[i];
        else
            ret += input[i] * weights[i];
    }

    return (ret + PRECISION / 2) / PRECISION;
}

HiddenLayer::HiddenLayer(std::vector<int16_t> inputs, size_t NeuronCount)
{
    assert(inputs.size() % NeuronCount == 0);

    size_t WeightsPerNeuron = inputs.size() / NeuronCount;

    for (size_t i = 0; i < NeuronCount; i++)
    {
        neurons.push_back(Neuron(std::vector<int16_t>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1)));
    }

    for (size_t i = 0; i < WeightsPerNeuron - 1; i++)
    {
        for (size_t j = 0; j < NeuronCount; j++)
        {
            weightTranspose.push_back(neurons.at(j).weights.at(i));
        }
    }

    zeta = std::vector<int16_t>(NeuronCount, 0);
}

std::vector<int16_t> HiddenLayer::FeedForward(std::vector<int16_t>& input, bool UseReLU)
{
    for (size_t i = 0; i < neurons.size(); i++)
    {
        zeta[i] = neurons.at(i).FeedForward(input, UseReLU);
    }

    return zeta;
}

void HiddenLayer::ApplyDelta(std::vector<deltaPoint>& deltaVec)
{
    size_t neuronCount = zeta.size();
    size_t deltaCount = deltaVec.size();

    for (size_t index = 0; index < deltaCount; index++)
    {
        int16_t deltaValue = deltaVec[index].delta;
        size_t weightTransposeIndex = deltaVec[index].index * neuronCount;

        for (size_t neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += weightTranspose[weightTransposeIndex + neuron] * deltaValue;
        }
    }
}

Network::Network(std::vector<std::vector<int16_t>> inputs, std::vector<size_t> NeuronCount) : outputNeuron(std::vector<int16_t>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    assert(inputs.size() == NeuronCount.size());

    inputNeurons = NeuronCount.at(0);

    for (size_t i = 1; i < NeuronCount.size() - 1; i++)
    {
        hiddenLayers.push_back(HiddenLayer(inputs.at(i), NeuronCount.at(i)));
    }

    for (size_t i = 0; i < 100; i++)
    {
        OldZeta.push_back(std::vector<int16_t>(hiddenLayers[0].neurons.size()));
    }
}

void Network::RecalculateIncremental(std::vector<int16_t> inputs)
{
    OldZeta.clear();
    for (size_t i = 0; i < 100; i++)
    {
        OldZeta.push_back(std::vector<int16_t>(hiddenLayers[0].neurons.size()));
    }
    incrementalDepth = 0;

    assert(inputs.size() == inputNeurons);

    //We never actually use FeedForward to get the evaluaton, only to 'refresh' the incremental updates and so we only need to do connection with first layer
    hiddenLayers.at(0).FeedForward(inputs, false);
}

void Network::ApplyDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    OldZeta[incrementalDepth++] = hiddenLayers[0].zeta;
    hiddenLayers[0].ApplyDelta(delta);
}

void Network::ApplyInverseDelta()
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].zeta = OldZeta[--incrementalDepth];
}

int16_t Network::QuickEval()
{
    std::vector<int16_t> inputs = hiddenLayers.at(0).zeta;

    for (size_t i = 1; i < hiddenLayers.size(); i++)    //skip first layer
    {
        inputs = hiddenLayers.at(i).FeedForward(inputs, true);
    }

    return (outputNeuron.FeedForward(inputs, true) + PRECISION / 2) / PRECISION;
}