#include "Network.h"

static const char* WeightsTXT[] = {
    #include "epoch1500_b8192_quant128.nn"
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
Neuron<INPUT_COUNT>::Neuron() : weights(),  bias(0)
{
}

template<size_t INPUT_COUNT>
Neuron<INPUT_COUNT>::Neuron(std::vector<int16_t> Weight, int16_t Bias) : weights(), bias(Bias)
{
    assert(Weight.size() == INPUT_COUNT);

    for (size_t i = 0; i < INPUT_COUNT; i++)
    {
        weights[i] = Weight[i];
    }
}

template<size_t INPUT_COUNT>
int32_t Neuron<INPUT_COUNT>::FeedForward(std::array<int16_t, INPUT_COUNT>& input) const
{
    int32_t ret = bias * PRECISION;

    for (size_t i = 0; i < INPUT_COUNT; i++)
    {
        ret += input[i] * weights[i];
    }

    return (ret + HALF_PRECISION) / PRECISION;
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
std::array<int16_t, OUTPUT_COUNT> HiddenLayer<INPUT_COUNT, OUTPUT_COUNT>::FeedForward(std::array<int16_t, INPUT_COUNT>& input)
{
    for (size_t i = 0; i < neurons->size(); i++)
    {
        zeta[i] = neurons->at(i).FeedForward(input);
    }

    return zeta;
}

template<size_t INPUT_COUNT, size_t OUTPUT_COUNT>
void HiddenLayer<INPUT_COUNT, OUTPUT_COUNT>::ApplyDelta(deltaArray& deltaVec)
{
    for (size_t point = 0; point < deltaVec.size; point++)
    {
        int16_t deltaValue = deltaVec.deltas[point].delta; 
        size_t weightTransposeIndex = deltaVec.deltas[point].index * OUTPUT_COUNT;

        if (deltaValue == 1)
        {
            for (size_t neuron = 0; neuron < OUTPUT_COUNT; neuron++)
            {
                zeta[neuron] += (*weightTranspose)[weightTransposeIndex + neuron];
            }
        }

        if (deltaValue == -1)
        {
            for (size_t neuron = 0; neuron < OUTPUT_COUNT; neuron++)
            {
                zeta[neuron] -= (*weightTranspose)[weightTransposeIndex + neuron];
            }
        }
    }
}

Network::Network(const std::vector<std::vector<int16_t>>& inputs) : hiddenLayer(inputs[1]), outputNeuron(std::vector<int16_t>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back()), OldZeta()
{
}

void Network::RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs)
{
    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        OldZeta[i] = {};
    }
    incrementalDepth = 0;

    //We never actually use FeedForward to get the evaluaton, only to 'refresh' the incremental updates and so we only need to do connection with first layer
    hiddenLayer.FeedForward(inputs);
}

void Network::ApplyDelta(deltaArray& delta)
{
    OldZeta[incrementalDepth++] = hiddenLayer.zeta;
    hiddenLayer.ApplyDelta(delta);
}

void Network::ApplyInverseDelta()
{
    hiddenLayer.zeta = OldZeta[--incrementalDepth];
}

int16_t Network::QuickEval()
{
    std::array<int16_t, HIDDEN_NEURONS> inputs;
    
    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        inputs[i] = std::max(int16_t(0), hiddenLayer.zeta[i]);
    }

    return (outputNeuron.FeedForward(inputs) + HALF_PRECISION) / PRECISION;
}