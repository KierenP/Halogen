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

    return Network(weights);
}

template<size_t INPUT_COUNT>
Neuron<INPUT_COUNT>::Neuron()
{
}

template<size_t INPUT_COUNT>
Neuron<INPUT_COUNT>::Neuron(std::vector<int16_t> Weight, int16_t Bias)
{
    assert(Weight.size() == INPUT_COUNT);

    bias = Bias;

    for (size_t i = 0; i < INPUT_COUNT; i++)
    {
        weights[i] = Weight[i];
    }
}

template<size_t INPUT_COUNT>
int32_t Neuron<INPUT_COUNT>::FeedForward(std::array<int16_t, INPUT_COUNT>& input, bool UseReLU) const
{
    assert(input.size() == weights.size());

    int32_t ret = bias << PRECISION_SHIFT;

    for (size_t i = 0; i < input.size(); i++)
    {
        if (UseReLU)
            ret += std::max(int16_t(0), input[i]) * weights[i];
        else
            ret += input[i] * weights[i];
    }

    return (ret + HALF_PRECISION) >> PRECISION_SHIFT;
}

template<size_t INPUT_COUNT, size_t OUTPUT_COUNT>
HiddenLayer<INPUT_COUNT, OUTPUT_COUNT>::HiddenLayer(std::vector<int16_t> inputs) : neurons(new std::array<Neuron<INPUT_COUNT>, OUTPUT_COUNT>), weightTranspose(new std::array<int16_t, INPUT_COUNT* OUTPUT_COUNT>)
{
    assert(inputs.size() % OUTPUT_COUNT == 0);

    size_t WeightsPerNeuron = inputs.size() / OUTPUT_COUNT;

    for (size_t i = 0; i < OUTPUT_COUNT; i++)
    {
        (*neurons)[i] = Neuron<INPUT_COUNT>(std::vector<int16_t>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1));
    }

    for (size_t i = 0; i < WeightsPerNeuron - 1; i++)
    {
        for (size_t j = 0; j < OUTPUT_COUNT; j++)
        {
            weightTranspose->at(i * OUTPUT_COUNT + j) = (neurons->at(j).weights.at(i));
        }
    }

    zeta = {};
}

template<size_t INPUT_COUNT, size_t OUTPUT_COUNT>
std::array<int16_t, OUTPUT_COUNT> HiddenLayer<INPUT_COUNT, OUTPUT_COUNT>::FeedForward(std::array<int16_t, INPUT_COUNT>& input, bool UseReLU)
{
    for (size_t i = 0; i < neurons->size(); i++)
    {
        zeta[i] = neurons->at(i).FeedForward(input, UseReLU);
    }

    return zeta;
}

template<size_t INPUT_COUNT, size_t OUTPUT_COUNT>
void HiddenLayer<INPUT_COUNT, OUTPUT_COUNT>::ApplyDelta(std::vector<deltaPoint>& deltaVec)
{
    size_t neuronCount = zeta.size();
    size_t deltaCount = deltaVec.size();

    for (size_t index = 0; index < deltaCount; index++)
    {
        int16_t deltaValue = deltaVec[index].delta; 
        size_t weightTransposeIndex = deltaVec[index].index * neuronCount;

        for (size_t neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += (*weightTranspose)[weightTransposeIndex + neuron] * deltaValue;
        }
    }
}

Network::Network(std::vector<std::vector<int16_t>> inputs) : hiddenLayer(inputs[1]), outputNeuron(std::vector<int16_t>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        OldZeta[i] = {};
    }
}

void Network::RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs)
{
    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        OldZeta[i] = {};
    }
    incrementalDepth = 0;

    assert(inputs.size() == inputNeurons);

    //We never actually use FeedForward to get the evaluaton, only to 'refresh' the incremental updates and so we only need to do connection with first layer
    hiddenLayer.FeedForward(inputs, false);
}

void Network::ApplyDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    OldZeta[incrementalDepth++] = hiddenLayer.zeta;
    hiddenLayer.ApplyDelta(delta);
}

void Network::ApplyInverseDelta()
{
    assert(hiddenLayers.size() > 0);
    hiddenLayer.zeta = OldZeta[--incrementalDepth];
}

int16_t Network::QuickEval()
{
    std::array<int16_t, HIDDEN_NEURONS> inputs = hiddenLayer.zeta;
    return (outputNeuron.FeedForward(inputs, true) + HALF_PRECISION) >> PRECISION_SHIFT;
}