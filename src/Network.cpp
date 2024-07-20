#include "Network.h"

#include <algorithm>
#include <cstdint>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
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

int get_king_bucket(Square king_sq, Players view)
{
    king_sq = view == WHITE ? king_sq : MirrorVertically(king_sq);
    return KING_BUCKETS[king_sq];
}

int index(Square king_sq, Square piece_sq, Pieces piece, Players view)
{
    piece_sq = view == WHITE ? piece_sq : MirrorVertically(piece_sq);

    auto king_bucket = get_king_bucket(king_sq, view);
    Players relativeColor = static_cast<Players>(view != ColourOfPiece(piece));
    PieceTypes pieceType = GetPieceType(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

void HalfAccumulator::AddInput(Square w_king, Square b_king, Square sq, Pieces piece)
{
    size_t w_index = index(w_king, sq, piece, WHITE);
    size_t b_index = index(b_king, sq, piece, BLACK);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        side[WHITE][j] += net.hiddenWeights[w_index][j];
        side[BLACK][j] += net.hiddenWeights[b_index][j];
    }
}

void HalfAccumulator::SubInput(Square w_king, Square b_king, Square sq, Pieces piece)
{
    size_t w_index = index(w_king, sq, piece, WHITE);
    size_t b_index = index(b_king, sq, piece, BLACK);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        side[WHITE][j] -= net.hiddenWeights[w_index][j];
        side[BLACK][j] -= net.hiddenWeights[b_index][j];
    }
}

void HalfAccumulator::Recalculate(const BoardState& board)
{
    side = { net.hiddenBias, net.hiddenBias };
    auto w_king = board.GetKing(WHITE);
    auto b_king = board.GetKing(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            AddInput(w_king, b_king, sq, piece);
        }
    }
}

void Network::Recalculate(const BoardState& board)
{
    AccumulatorStack.clear();
    AccumulatorStack.emplace_back().Recalculate(board);
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
            expected.AddInput(w_king, b_king, sq, piece);
        }
    }

    return expected == AccumulatorStack.back();
}

void Network::ApplyMove(const BoardState& prev_move_board, const BoardState& post_move_board, Move move)
{
    auto& acc = AccumulatorStack.emplace_back(AccumulatorStack.back());

    auto stm = prev_move_board.stm;
    auto from_sq = move.GetFrom();
    auto to_sq = move.GetTo();
    auto from_piece = prev_move_board.GetSquare(from_sq);
    auto to_piece = post_move_board.GetSquare(to_sq);
    auto cap_piece = prev_move_board.GetSquare(to_sq);

    auto w_king = post_move_board.GetKing(WHITE);
    auto b_king = post_move_board.GetKing(BLACK);

    if (from_piece == Piece(KING, stm))
    {
        if (get_king_bucket(from_sq, stm) != get_king_bucket(to_sq, stm))
        {
            // king bucket has changed -> recalculate the accumulator.
            // TODO: avoid recalculating the non-STM accumulator
            acc.Recalculate(post_move_board);
        }
        else if (move.IsCastle())
        {
            Square king_start = move.GetFrom();
            Square king_end
                = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
            Square rook_start = move.GetTo();
            Square rook_end
                = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

            acc.AddInput(w_king, b_king, king_end, from_piece);
            acc.SubInput(w_king, b_king, king_start, from_piece);
            acc.AddInput(w_king, b_king, rook_end, cap_piece);
            acc.SubInput(w_king, b_king, rook_start, cap_piece);
        }
        else if (move.IsCapture())
        {
            acc.AddInput(w_king, b_king, to_sq, from_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
            acc.SubInput(w_king, b_king, to_sq, cap_piece);
        }
        else
        {
            acc.AddInput(w_king, b_king, to_sq, from_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
        }
    }
    else
    {
        if (move.IsPromotion() && move.IsCapture())
        {
            acc.AddInput(w_king, b_king, to_sq, to_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
            acc.SubInput(w_king, b_king, to_sq, cap_piece);
        }
        else if (move.IsPromotion())
        {
            acc.AddInput(w_king, b_king, to_sq, to_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
        }
        else if (move.IsCapture() && move.GetFlag() == EN_PASSANT)
        {
            auto ep_capture_sq = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));
            acc.AddInput(w_king, b_king, to_sq, from_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
            acc.SubInput(w_king, b_king, ep_capture_sq, Piece(PAWN, !stm));
        }
        else if (move.IsCapture())
        {
            acc.AddInput(w_king, b_king, to_sq, from_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
            acc.SubInput(w_king, b_king, to_sq, cap_piece);
        }
        else
        {
            acc.AddInput(w_king, b_king, to_sq, from_piece);
            acc.SubInput(w_king, b_king, from_sq, from_piece);
        }
    }
}

void Network::UndoMove()
{
    AccumulatorStack.pop_back();
}

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
