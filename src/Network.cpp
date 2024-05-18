#include "Network.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "incbin/incbin.h"

INCBIN(Net, EVALFILE);

alignas(32) std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> Network::hiddenWeights = {};
alignas(32) std::array<int16_t, HIDDEN_NEURONS> Network::hiddenBias = {};
alignas(32) std::array<int16_t, HIDDEN_NEURONS * 2> Network::outputWeights = {};
alignas(32) int16_t Network::outputBias = {};

constexpr int16_t L1_SCALE = 128;
constexpr int16_t L2_SCALE = 128;
constexpr double SCALE_FACTOR = 1; // Found empirically to maximize elo

template <typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> ReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::max(T(0), source[i]);

    return ret;
}

template <typename T_out, typename T_in, size_t SIZE>
void DotProductHalves(const std::array<T_in, SIZE>& stm, const std::array<T_in, SIZE>& other,
    const std::array<T_in, SIZE * 2>& weights, T_out& output)
{
    for (size_t i = 0; i < SIZE; i++)
    {
        output += stm[i] * weights[i];
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        output += other[i] * weights[i + SIZE];
    }
}

void Network::Init()
{
    auto Data = reinterpret_cast<const float*>(gNetData);

    float l1_weight_min = std::numeric_limits<float>::max();
    float l1_weight_max = std::numeric_limits<float>::min();

    float l1_bias_min = std::numeric_limits<float>::max();
    float l1_bias_max = std::numeric_limits<float>::min();

    float l2_weight_min = std::numeric_limits<float>::max();
    float l2_weight_max = std::numeric_limits<float>::min();

    float l2_bias_min = std::numeric_limits<float>::max();
    float l2_bias_max = std::numeric_limits<float>::min();

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        for (size_t j = 0; j < INPUT_NEURONS; j++)
        {
            auto val = *Data++;
            l1_weight_min = std::min(l1_weight_min, val);
            l1_weight_max = std::max(l1_weight_max, val);
            hiddenWeights[j][i] = (int16_t)round(val * L1_SCALE);
        }
    }

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
    {
        auto val = *Data++;
        l1_bias_min = std::min(l1_bias_min, val);
        l1_bias_max = std::max(l1_bias_max, val);
        hiddenBias[i] = (int16_t)round(val * L1_SCALE);
    }

    for (size_t i = 0; i < HIDDEN_NEURONS * 2; i++)
    {
        auto val = *Data++;
        l2_weight_min = std::min(l2_weight_min, val);
        l2_weight_max = std::max(l2_weight_max, val);
        outputWeights[i] = (int16_t)round(val * SCALE_FACTOR * L2_SCALE);
    }

    auto val = *Data++;
    l2_bias_min = std::min(l2_bias_min, val);
    l2_bias_max = std::max(l2_bias_max, val);
    outputBias = (int16_t)round(val * SCALE_FACTOR * L2_SCALE);

    std::cout << "l1_weight_min: " << l1_weight_min << std::endl;
    std::cout << "l1_weight_max: " << l1_weight_max << std::endl;
    std::cout << "l1_bias_min: " << l1_bias_min << std::endl;
    std::cout << "l1_bias_max: " << l1_bias_max << std::endl;
    std::cout << "l2_weight_min: " << l2_weight_min << std::endl;
    std::cout << "l2_weight_max: " << l2_weight_max << std::endl;
    std::cout << "l2_bias_min: " << l2_bias_min << std::endl;
    std::cout << "l2_bias_max: " << l2_bias_max << std::endl;

    assert(reinterpret_cast<const unsigned char*>(Data) == gNetData + gNetSize);
}

void Network::Recalculate(const BoardState& board)
{
    AccumulatorStack = { { hiddenBias, hiddenBias } };

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            AddInput(sq, piece);
        }
    }
}

Square MirrorVertically(Square sq)
{
    return static_cast<Square>(sq ^ 56);
}

int Network::index(Square square, Pieces piece, Players view)
{
    Square sq = view == WHITE ? square : MirrorVertically(square);
    Pieces relativeColor = static_cast<Pieces>(view == ColourOfPiece(piece));
    PieceTypes pieceType = GetPieceType(piece);

    return sq + pieceType * 64 + relativeColor * 64 * 6;
}

bool Network::Verify(const BoardState& board) const
{
    HalfAccumulator correct_answer = { hiddenBias, hiddenBias };

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            size_t white_index = index(sq, piece, WHITE);
            size_t black_index = index(sq, piece, BLACK);

            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            {
                correct_answer.side[WHITE][j] += hiddenWeights[white_index][j];
                correct_answer.side[BLACK][j] += hiddenWeights[black_index][j];
            }
        }
    }

    return correct_answer == AccumulatorStack.back();
}

void Network::AccumulatorPush()
{
    AccumulatorStack.push_back(AccumulatorStack.back());
}

void Network::AccumulatorPop()
{
    AccumulatorStack.pop_back();
}

void Network::AddInput(Square square, Pieces piece)
{
    size_t white_index = index(square, piece, WHITE);
    size_t black_index = index(square, piece, BLACK);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        AccumulatorStack.back().side[WHITE][j] += hiddenWeights[white_index][j];
        AccumulatorStack.back().side[BLACK][j] += hiddenWeights[black_index][j];
    }
}

void Network::RemoveInput(Square square, Pieces piece)
{
    size_t white_index = index(square, piece, WHITE);
    size_t black_index = index(square, piece, BLACK);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        AccumulatorStack.back().side[WHITE][j] -= hiddenWeights[white_index][j];
        AccumulatorStack.back().side[BLACK][j] -= hiddenWeights[black_index][j];
    }
}

Score Network::Eval(Players stm) const
{
    int32_t output = outputBias * L1_SCALE;
    DotProductHalves(
        ReLU(AccumulatorStack.back().side[stm]), ReLU(AccumulatorStack.back().side[!stm]), outputWeights, output);
    output /= L1_SCALE * L2_SCALE;
    return output;
}
