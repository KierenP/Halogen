#include "Network.h"
#include "epoch170_b16384_768-256-1_embedded.nn"

std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>* hiddenWeights;
std::array<int16_t, HIDDEN_NEURONS>* hiddenBias;
std::array<int16_t, HIDDEN_NEURONS>* outputWeights;
int16_t* outputBias;

void NetworkInit()
{
    hiddenWeights = new std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS>;
    hiddenBias    = new std::array<int16_t, HIDDEN_NEURONS>;
    outputWeights = new std::array<int16_t, HIDDEN_NEURONS>;
    outputBias    = new int16_t;

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
            (*hiddenWeights)[i][j] = (int16_t)round(HiddenWeights[i * HIDDEN_NEURONS + j] * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        (*hiddenBias)[i] = (int16_t)round(HiddenBias[i] * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        (*outputWeights)[i] = (int16_t)round(OutputWeights[i] * PRECISION);

    (*outputBias) = (int16_t)round(OutputBias[0] * PRECISION);

    delete[] HiddenWeights;
    delete[] HiddenBias;
    delete[] OutputWeights;
    delete[] OutputBias;
}

void RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs, std::array<std::array<int32_t, HIDDEN_NEURONS>, MAX_DEPTH>& Zeta, size_t& incrementalDepth)
{
    incrementalDepth = 0;

    for (size_t i = 0; i < MAX_DEPTH; i++)
        Zeta[i] = {};

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        Zeta[0][i] = (*hiddenBias)[i] * PRECISION;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        for (size_t j = 0; j < INPUT_NEURONS; j++)
            Zeta[0][i] += inputs[j] * (*hiddenWeights)[j][i];

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        Zeta[0][i] = (Zeta[0][i] + HALF_PRECISION) / PRECISION;
}

void ApplyDelta(deltaArray& update, std::array<std::array<int32_t, HIDDEN_NEURONS>, MAX_DEPTH>& Zeta, size_t& incrementalDepth)
{
    incrementalDepth++;
    Zeta[incrementalDepth] = Zeta[incrementalDepth - 1];

    for (size_t i = 0; i < update.size; i++)
    {
        if (update.deltas[i].delta == 1)
            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                Zeta[incrementalDepth][j] += (*hiddenWeights)[update.deltas[i].index][j];
        else
            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
                Zeta[incrementalDepth][j] -= (*hiddenWeights)[update.deltas[i].index][j];
    }
}

void ApplyInverseDelta(size_t& incrementalDepth)
{
    --incrementalDepth;
}

int16_t QuickEval(const std::array<std::array<int32_t, HIDDEN_NEURONS>, MAX_DEPTH>& Zeta, const size_t& incrementalDepth)
{
    int32_t output = (*outputBias) * PRECISION;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        output += std::max(int32_t(0), Zeta[incrementalDepth][i]) * (*outputWeights)[i];

    output += HALF_PRECISION;
    output /= PRECISION;
    output += HALF_PRECISION;
    output /= PRECISION;
    return output;
}