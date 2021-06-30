#pragma once
#include <array>
#include <vector>
#include "BitBoardDefine.h"

//HalfKP 64x2-32-32

constexpr size_t INPUT_NEURONS = 64 * 64 * 10;
constexpr size_t HALF_L1 = 64;
constexpr size_t L1_NEURONS = HALF_L1 * 2;
constexpr size_t L2_NEURONS = 32;
constexpr size_t L3_NEURONS = 32;
constexpr size_t OUTPUT_NEURONS = 1;

class Position;

class Network
{
public:
    int16_t Eval(const Position& position) const;

    static void Init();

private:
    static std::array<float, HALF_L1> l1_bias;
    static std::array<std::array<float, HALF_L1>, INPUT_NEURONS> l1_weight;

    static std::array<float, L2_NEURONS> l2_bias;
    static std::array<std::array<float, L2_NEURONS>, L1_NEURONS> l2_weight;

    static std::array<float, L3_NEURONS> l3_bias;
    static std::array<std::array<float, L3_NEURONS>, L2_NEURONS> l3_weight;

    static std::array<float, OUTPUT_NEURONS> out_bias;
    static std::array<std::array<float, OUTPUT_NEURONS>, L3_NEURONS> out_weight;
};
