#include "Network.h"
#include "halogen-x256-eb873cf4.nn"

std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> Network::hiddenWeights = {};
std::array<int16_t, HIDDEN_NEURONS> Network::hiddenBias = {};
std::array<int16_t, HIDDEN_NEURONS> Network::outputWeights = {};
int16_t Network::outputBias = {};

void Network::Init()
{
    auto* HiddenWeights = new float[INPUT_NEURONS * HIDDEN_NEURONS];
    auto* HiddenBias    = new float[HIDDEN_NEURONS];
    auto* OutputWeights = new float[HIDDEN_NEURONS];
    auto* OutputBias    = new float[1];

    memcpy(HiddenBias,    &label[0],                                                                     sizeof(float) * HIDDEN_NEURONS);
    memcpy(HiddenWeights, &label[(HIDDEN_NEURONS) * sizeof(float)],                                      sizeof(float) * INPUT_NEURONS * HIDDEN_NEURONS);
    memcpy(OutputBias,    &label[(HIDDEN_NEURONS + INPUT_NEURONS * HIDDEN_NEURONS) * sizeof(float)],     sizeof(float) * 1);
    memcpy(OutputWeights, &label[(HIDDEN_NEURONS + INPUT_NEURONS * HIDDEN_NEURONS + 1) * sizeof(float)], sizeof(float) * HIDDEN_NEURONS);

    for (size_t i = 0; i < INPUT_NEURONS; i++)
        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            hiddenWeights[i][j] = (int16_t)round(HiddenWeights[i * HIDDEN_NEURONS + j] * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        hiddenBias[i] = (int16_t)round(HiddenBias[i] * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        outputWeights[i] = (int16_t)round(OutputWeights[i] * PRECISION);

    outputBias = (int16_t)round(OutputBias[0] * PRECISION);

    delete[] HiddenWeights;
    delete[] HiddenBias;
    delete[] OutputWeights;
    delete[] OutputBias;
}

void Network::RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs)
{
    Zeta.resize(1);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        Zeta[0][i] = hiddenBias[i];

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        for (size_t j = 0; j < INPUT_NEURONS; j++)
            Zeta[0][i] += inputs[j] * hiddenWeights[j][i];
}

void Network::ApplyDelta(const deltaArray& update)
{
    Zeta.push_back(Zeta.back());

    for (size_t i = 0; i < update.size; i++)
    {
        if (update.deltas[i].delta == 1)
            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                Zeta.back()[j] += hiddenWeights[update.deltas[i].index][j];
        else
            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                Zeta.back()[j] -= hiddenWeights[update.deltas[i].index][j];
    }
}

void Network::ApplyInverseDelta()
{
    Zeta.pop_back();
}

int16_t Network::QuickEval() const
{
    int32_t output = outputBias * PRECISION;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        output += std::max(int16_t(0), Zeta.back()[i]) * outputWeights[i];

    return output / SQUARE_PRECISION;
}