#include "Network.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "incbin/incbin.h"

INCBIN(Net, EVALFILE);

alignas(32) std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS> Network::hiddenWeights = {};
alignas(32) std::array<int16_t, HIDDEN_NEURONS> Network::hiddenBias = {};
alignas(32) std::array<int16_t, HIDDEN_NEURONS * 2> Network::outputWeights = {};
alignas(32) int16_t Network::outputBias = {};

constexpr int16_t L1_SCALE = 255;
constexpr int16_t L2_SCALE = 64;
constexpr double SCALE_FACTOR = 400; // Found empirically to maximize elo

template <typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> ReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::max(T(0), source[i]);

    return ret;
}

template <typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> CReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::clamp(source[i], T(0), T(L1_SCALE));

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
    auto Data = reinterpret_cast<const int16_t*>(gNetData);

    for (size_t i = 0; i < INPUT_NEURONS; i++)
        for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            hiddenWeights[i][j] = *Data++;

    for (size_t i = 0; i < HIDDEN_NEURONS; i++)
        hiddenBias[i] = *Data++;

    for (size_t i = 0; i < HIDDEN_NEURONS * 2; i++)
        outputWeights[i] = *Data++;

    outputBias = *Data++;

    // bullet trainer pads to a multiple of 64 bytes
    Data += (64 - ((reinterpret_cast<const unsigned char*>(Data) - gNetData) % 64)) / sizeof(int16_t);
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
    Players relativeColor = static_cast<Players>(view != ColourOfPiece(piece));
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
    int32_t output = outputBias;
    DotProductHalves(
        CReLU(AccumulatorStack.back().side[stm]), CReLU(AccumulatorStack.back().side[!stm]), outputWeights, output);
    output *= SCALE_FACTOR;
    output /= L1_SCALE * L2_SCALE;
    return output;
}
