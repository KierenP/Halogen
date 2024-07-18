#include "Network.h"

#include <algorithm>
#include <cstdint>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "incbin/incbin.h"

constexpr size_t OUTPUT_BUCKETS = 8;

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 0, 1, 1, 2, 2, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
    6, 6, 6, 6, 7, 7, 7, 7,
};
// clang-format on

constexpr size_t KING_BUCKET_COUNT = []()
{
    auto [min, max] = std::minmax_element(KING_BUCKETS.begin(), KING_BUCKETS.end());
    return *max - *min + 1;
}();

INCBIN(Net, EVALFILE);

struct alignas(64) network
{
    std::array<std::array<int16_t, HIDDEN_NEURONS>, INPUT_NEURONS * KING_BUCKET_COUNT> hiddenWeights = {};
    std::array<int16_t, HIDDEN_NEURONS> hiddenBias = {};
    std::array<std::array<int16_t, HIDDEN_NEURONS * 2>, OUTPUT_BUCKETS> outputWeights = {};
    std::array<int16_t, OUTPUT_BUCKETS> outputBias = {};
} const& net = reinterpret_cast<const network&>(*gNetData);

[[maybe_unused]] auto verify_network_size = []
{
    assert(sizeof(network) == gNetSize);
    return true;
}();

constexpr int16_t L1_SCALE = 181;
constexpr int16_t L2_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

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

template <typename T, size_t SIZE>
[[nodiscard]] std::array<T, SIZE> SCReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
    {
        ret[i] = std::clamp(source[i], T(0), T(L1_SCALE));
        ret[i] = ret[i] * ret[i] / L1_SCALE;
    }

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

Square MirrorVertically(Square sq)
{
    return static_cast<Square>(sq ^ 56);
}

int index(Square king_sq, Square piece_sq, Pieces piece, Players view)
{
    piece_sq = view == WHITE ? piece_sq : MirrorVertically(piece_sq);
    king_sq = view == WHITE ? king_sq : MirrorVertically(king_sq);

    auto king_bucket = KING_BUCKETS[king_sq];
    Players relativeColor = static_cast<Players>(view != ColourOfPiece(piece));
    PieceTypes pieceType = GetPieceType(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

void Network::Recalculate(const BoardState& board)
{
    AccumulatorStack = { { net.hiddenBias, net.hiddenBias } };
    auto w_king = board.GetKing(WHITE);
    auto b_king = board.GetKing(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            size_t w_index = index(w_king, sq, piece, WHITE);
            size_t b_index = index(b_king, sq, piece, BLACK);

            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            {
                AccumulatorStack.back().side[WHITE][j] += net.hiddenWeights[w_index][j];
                AccumulatorStack.back().side[BLACK][j] += net.hiddenWeights[b_index][j];
            }
        }
    }
}

bool Network::Verify(const BoardState& board) const
{
    HalfAccumulator expected = { net.hiddenBias, net.hiddenBias };
    auto w_king = board.GetKing(WHITE);
    auto b_king = board.GetKing(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            size_t w_index = index(w_king, sq, piece, WHITE);
            size_t b_index = index(b_king, sq, piece, WHITE);

            for (size_t j = 0; j < HIDDEN_NEURONS; j++)
            {
                expected.side[WHITE][j] += net.hiddenWeights[w_index][j];
                expected.side[BLACK][j] += net.hiddenWeights[b_index][j];
            }
        }
    }

    return expected == AccumulatorStack.back();
}

/*void Network::ApplyMove(const BoardState&, Move)
{
    // TODO
}

void Network::UndoMove(const BoardState&)
{
    // TODO
}*/

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

Score Network::Eval(const BoardState& board) const
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(GetBitCount(board.GetAllPieces()));

    int32_t output = net.outputBias[output_bucket];
    DotProductHalves(SCReLU(AccumulatorStack.back().side[stm]), SCReLU(AccumulatorStack.back().side[!stm]),
        net.outputWeights[output_bucket], output);
    output *= SCALE_FACTOR;
    output /= L1_SCALE * L2_SCALE;
    return output;
}
