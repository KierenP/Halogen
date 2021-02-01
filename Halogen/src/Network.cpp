#include "Network.h"
#include "epoch5.net"

std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> Network::hiddenWeights = {};
std::array<int16_t, HIDDEN_NEURONS> Network::hiddenBias = {};
std::array<int16_t, HIDDEN_NEURONS> Network::outputWeights = {};
int16_t Network::outputBias = {};

template<typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> ReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::max(T(0), source[i]);
    
    return ret;
}

template<typename T_out, typename T_in, size_t SIZE>
void DotProduct(const std::array<T_in, SIZE>& a, const std::array<T_in, SIZE>& b, T_out& output)
{
    for (size_t i = 0; i < SIZE; i++)
        output += a[i] * b[i];
}

void Network::Init()
{
    auto Data = reinterpret_cast<float*>(label);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        hiddenBias[i] = (int16_t)round(*Data++ * PRECISION);

    for (size_t i = 0; i < INPUT_NEURONS; i++)
        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            hiddenWeights[i][j] = (int16_t)round(*Data++ * PRECISION);

    outputBias = (int16_t)round(*Data++ * PRECISION);

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        outputWeights[i] = (int16_t)round(*Data++ * PRECISION);
}

void Network::RecalculateIncremental(std::array<int16_t, INPUT_NEURONS> inputs)
{
    Zeta = { hiddenBias };

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
    DotProduct(ReLU(Zeta.back()), outputWeights, output);
    return output / SQUARE_PRECISION;
}

/*void QuantizationAnalysis()
{
    auto Data = reinterpret_cast<float*>(label);

    float weight = 0;

    //hidden bias
    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        weight = std::max(weight, abs(*Data++));

    std::cout << weight << std::endl;
    weight = 0;

    //hidden weight
    for (size_t i = 0; i < INPUT_NEURONS; i++)
        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            weight = std::max(weight, abs(*Data++));

    std::cout << weight << std::endl;
    weight = 0;

    //output bias
    weight = std::max(weight, abs(*Data++));

    std::cout << weight << std::endl;
    weight = 0;

    //output weights
    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        weight = std::max(weight, abs(*Data++));

    std::cout << weight << std::endl;
    weight = 0;
}*/