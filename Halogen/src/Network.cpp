#include "Network.h"
#include "epoch170_b16384_x256_embedded.nn"

Network* InitNetwork()
{
    auto* HiddenWeights = new float[INPUT_NEURONS * HIDDEN_NEURONS];
    auto* HiddenBias = new float[HIDDEN_NEURONS];
    auto* OutputWeights = new float[HIDDEN_NEURONS];
    auto* OutputBias = new float[1];

    memcpy(HiddenBias,      &label[0],                                                                      sizeof(float) * HIDDEN_NEURONS);
    memcpy(HiddenWeights,   &label[(HIDDEN_NEURONS) * sizeof(float)],                                       sizeof(float) * INPUT_NEURONS * HIDDEN_NEURONS);
    memcpy(OutputBias,      &label[(HIDDEN_NEURONS + INPUT_NEURONS * HIDDEN_NEURONS) * sizeof(float)],      sizeof(float) * 1);
    memcpy(OutputWeights,   &label[(HIDDEN_NEURONS + INPUT_NEURONS * HIDDEN_NEURONS + 1) * sizeof(float)],  sizeof(float) * HIDDEN_NEURONS);

    auto* HiddenWeightsQuant = new std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>;
    auto* HiddenBiasQuant = new std::array<int16_t, HIDDEN_NEURONS>;
    auto* OutputWeightsQuant = new std::array<int16_t, HIDDEN_NEURONS>;
    auto* OutputBiasQuant = new int16_t;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        (*HiddenBiasQuant)[i] = (int16_t)round(HiddenBias[i] * PRECISION);
    }

    for (size_t i = 0; i < INPUT_NEURONS; i++)
    {
        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
        {
            (*HiddenWeightsQuant)[i][j] = (int16_t)round(HiddenWeights[i * HIDDEN_NEURONS + j] * PRECISION);
        }
    }

    (*OutputBiasQuant) = (int16_t)round(OutputBias[0] * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        (*OutputWeightsQuant)[i] = (int16_t)round(OutputWeights[i] * PRECISION);
    }

    auto ret = new Network(HiddenWeightsQuant, HiddenBiasQuant, OutputWeightsQuant, OutputBiasQuant);

    delete[] HiddenWeights;
    delete[] HiddenBias;
    delete[] OutputWeights;
    delete[] OutputBias;
    delete HiddenWeightsQuant;
    delete HiddenBiasQuant;
    delete OutputWeightsQuant;
    delete OutputBiasQuant;

    return ret;
}

Network::Network(std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>* HiddenWeights,
    std::array<int16_t, HIDDEN_NEURONS>* HiddenBias,
    std::array<int16_t, HIDDEN_NEURONS>* OutputWeights,
    int16_t* OutputBias) :
    hiddenWeights(*HiddenWeights),
    hiddenBias(*HiddenBias),
    outputWeights(*OutputWeights),
    outputBias(*OutputBias),
    Zeta() {}

void Network::RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs)
{
    for (size_t i = 0; i < MAX_DEPTH; i++)
    {
        Zeta[i] = {};
    }
    incrementalDepth = 0;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        Zeta[incrementalDepth][i] = hiddenBias[i];
        for (size_t j = 0; j < INPUT_NEURONS; j++)
        {
            Zeta[incrementalDepth][i] += inputs[j] * hiddenWeights[j][i];
        }
    }
}

void Network::ApplyDelta(deltaArray& update)
{
    incrementalDepth++;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        Zeta[incrementalDepth][i] = Zeta[incrementalDepth - 1][i];
    }

    for (size_t i = 0; i < update.size; i++)
    {
        size_t index = update.deltas[i].index;
        int16_t delta = update.deltas[i].delta;

        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
        {
            Zeta[incrementalDepth][j] += delta * hiddenWeights[index][j];
        }
    }
}

void Network::ApplyInverseDelta()
{
    --incrementalDepth;
}

int16_t Network::QuickEval()
{
    int32_t output = outputBias * PRECISION;
    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        output += std::max(int32_t(0), Zeta[incrementalDepth][i]) * outputWeights[i];
    }
    output += PRECISION * PRECISION / 2;
    output /= PRECISION * PRECISION;
    return output;
}