#pragma once
#include <array>
#include <vector>
#include <iostream>
#include "BitBoardDefine.h"

//HalfKP 64x2-32-32 WDL

constexpr size_t INPUT_NEURONS = 64 * 64 * 10;
constexpr size_t HALF_L1 = 64;
constexpr size_t L1_NEURONS = HALF_L1 * 2;
constexpr size_t L2_NEURONS = 32;
constexpr size_t L3_NEURONS = 32;
constexpr size_t OUTPUT_NEURONS = 3;

class Position;

struct WDL
{
    WDL() : win(-1), draw(-1), loss(-1) {}

    WDL(float win_, float draw_, float loss_) : win(win_), draw(draw_), loss(loss_)
    {
        assert(abs(1 - win - draw - loss) < 0.001);
    }

    WDL(std::array<float, 3> data) : WDL(data[0], data[1], data[2]) {}

    float win;
    float draw;
    float loss;

    int16_t ToCP();

    friend std::ostream& operator<<(std::ostream& os, WDL const& wdl) {
        return os << wdl.win << " " << wdl.draw << " " << wdl.loss;
    }

    static const WDL WIN;
    static const WDL DRAW;
    static const WDL LOSS;
};

inline const WDL WDL::WIN  = WDL(1, 0, 0);
inline const WDL WDL::DRAW = WDL(0, 1, 0);
inline const WDL WDL::LOSS = WDL(0, 0, 1);

class Network
{
public:
    //Get the score in cp from white's POV
    int16_t Eval(const Position& position) const;

    //Get the { W, D, L } probabilities from white's POV
    WDL EvalWDL(const Position& position) const;

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
