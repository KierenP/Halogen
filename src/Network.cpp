#include "Network.h"

#include <algorithm>
#include <cstdint>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "incbin/incbin.h"

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

Square MirrorHorizontally(Square sq)
{
    return static_cast<Square>(sq ^ 7);
}

int get_king_bucket(Square king_sq, Players view)
{
    king_sq = view == WHITE ? king_sq : MirrorVertically(king_sq);
    return KING_BUCKETS[king_sq];
}

int index(Square king_sq, Square piece_sq, Pieces piece, Players view)
{
    piece_sq = view == WHITE ? piece_sq : MirrorVertically(piece_sq);
    piece_sq = GetFile(king_sq) <= FILE_D ? piece_sq : MirrorHorizontally(piece_sq);

    auto king_bucket = get_king_bucket(king_sq, view);
    Players relativeColor = static_cast<Players>(view != ColourOfPiece(piece));
    PieceTypes pieceType = GetPieceType(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const Input& add1, const Input& sub1, Players view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j];
    }
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1)
{
    Add1Sub1(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { sub1.w_king, sub1.piece_sq, sub1.piece }, WHITE);
    Add1Sub1(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { sub1.b_king, sub1.piece_sq, sub1.piece }, BLACK);
}

void Add1Sub2(
    const Accumulator& prev, Accumulator& next, const Input& add1, const Input& sub1, const Input& sub2, Players view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king_sq, sub2.piece_sq, sub2.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j]
            - net.hiddenWeights[sub2_index][j];
    }
}

void Add1Sub2(
    const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1, const InputPair& sub2)
{
    Add1Sub2(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { sub1.w_king, sub1.piece_sq, sub1.piece },
        { sub2.w_king, sub2.piece_sq, sub2.piece }, WHITE);
    Add1Sub2(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { sub1.b_king, sub1.piece_sq, sub1.piece },
        { sub2.b_king, sub2.piece_sq, sub2.piece }, BLACK);
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const Input& add1, const Input& add2, const Input& sub1,
    const Input& sub2, Players view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t add2_index = index(add2.king_sq, add2.piece_sq, add2.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king_sq, sub2.piece_sq, sub2.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j]
            + net.hiddenWeights[add2_index][j] - net.hiddenWeights[sub2_index][j];
    }
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& add2,
    const InputPair& sub1, const InputPair& sub2)
{
    Add2Sub2(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { add2.w_king, add2.piece_sq, add2.piece },
        { sub1.w_king, sub1.piece_sq, sub1.piece }, { sub2.w_king, sub2.piece_sq, sub2.piece }, WHITE);
    Add2Sub2(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { add2.b_king, add2.piece_sq, add2.piece },
        { sub1.b_king, sub1.piece_sq, sub1.piece }, { sub2.b_king, sub2.piece_sq, sub2.piece }, BLACK);
}

void Accumulator::AddInput(const InputPair& input)
{
    AddInput({ input.w_king, input.piece_sq, input.piece }, WHITE);
    AddInput({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::AddInput(const Input& input, Players view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        side[view][j] += net.hiddenWeights[side_index][j];
    }
}

void Accumulator::SubInput(const InputPair& input)
{
    SubInput({ input.w_king, input.piece_sq, input.piece }, WHITE);
    SubInput({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::SubInput(const Input& input, Players view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        side[view][j] -= net.hiddenWeights[side_index][j];
    }
}

void Accumulator::Recalculate(const BoardState& board)
{
    Recalculate(board, WHITE);
    Recalculate(board, BLACK);
}

void Accumulator::Recalculate(const BoardState& board, Players view)
{
    side[view] = net.hiddenBias;
    auto king = board.GetKing(view);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            AddInput({ king, sq, piece }, view);
        }
    }
}

void AccumulatorTable::Recalculate(Accumulator& acc, const BoardState& board, Players side, Square king_sq)
{
    // we need to keep a separate entry for when the king is on each side of the board
    auto& entry = king_bucket[get_king_bucket(king_sq, side) + (GetFile(king_sq) <= FILE_D ? 0 : KING_BUCKET_COUNT)];
    auto& bb = side == WHITE ? entry.white_bb : entry.black_bb;

    for (const auto& piece : {
             WHITE_PAWN,
             WHITE_KNIGHT,
             WHITE_BISHOP,
             WHITE_ROOK,
             WHITE_QUEEN,
             WHITE_KING,
             BLACK_PAWN,
             BLACK_KNIGHT,
             BLACK_BISHOP,
             BLACK_ROOK,
             BLACK_QUEEN,
             BLACK_KING,
         })
    {
        auto new_bb = board.GetPieceBB(piece);
        auto& old_bb = bb[piece];

        auto to_add = new_bb & ~old_bb;
        auto to_sub = old_bb & ~new_bb;

        while (to_add)
        {
            auto sq = LSBpop(to_add);
            entry.acc.AddInput({ king_sq, sq, piece }, side);
        }

        while (to_sub)
        {
            auto sq = LSBpop(to_sub);
            entry.acc.SubInput({ king_sq, sq, piece }, side);
        }

        old_bb = new_bb;
    }

    acc.side[side] = entry.acc.side[side];
}

void Network::Reset(const BoardState& board, Accumulator& acc)
{
    acc.Recalculate(board);

    for (auto& entry : table.king_bucket)
    {
        entry.acc = { net.hiddenBias, net.hiddenBias };
        entry.white_bb = {};
        entry.black_bb = {};
    }
}

bool Network::Verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = { net.hiddenBias, net.hiddenBias };
    auto w_king = board.GetKing(WHITE);
    auto b_king = board.GetKing(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            expected.AddInput({ w_king, b_king, sq, piece });
        }
    }

    return expected == acc;
}

void Network::ApplyMove(const BoardState& prev_move_board, const BoardState& post_move_board,
    const Accumulator& prev_acc, Accumulator& next_acc, Move move)
{
    assert(Verify(prev_move_board, prev_acc));

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
        if (get_king_bucket(from_sq, stm) != get_king_bucket(to_sq, stm)
            || ((GetFile(from_sq) <= FILE_D) ^ (GetFile(to_sq) <= FILE_D)))
        {
            auto our_king = stm == WHITE ? w_king : b_king;
            auto their_king = stm == WHITE ? b_king : w_king;

            // king bucket has changed -> recalculate that side's accumulator
            table.Recalculate(next_acc, post_move_board, stm, our_king);

            // incrementally update the other side's accumulator
            if (move.IsCastle())
            {
                Square king_start = move.GetFrom();
                Square king_end
                    = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
                Square rook_start = move.GetTo();
                Square rook_end
                    = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

                Add2Sub2(prev_acc, next_acc, { their_king, king_end, from_piece }, { their_king, rook_end, cap_piece },
                    { their_king, king_start, from_piece }, { their_king, rook_start, cap_piece }, !stm);
            }
            else if (move.IsCapture())
            {
                Add1Sub2(prev_acc, next_acc, { their_king, to_sq, from_piece }, { their_king, from_sq, from_piece },
                    { their_king, to_sq, cap_piece }, !stm);
            }
            else
            {
                Add1Sub1(
                    prev_acc, next_acc, { their_king, to_sq, from_piece }, { their_king, from_sq, from_piece }, !stm);
            }
        }
        else if (move.IsCastle())
        {
            Square king_start = move.GetFrom();
            Square king_end
                = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
            Square rook_start = move.GetTo();
            Square rook_end
                = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

            Add2Sub2(prev_acc, next_acc, { w_king, b_king, king_end, from_piece },
                { w_king, b_king, rook_end, cap_piece }, { w_king, b_king, king_start, from_piece },
                { w_king, b_king, rook_start, cap_piece });
        }
        else if (move.IsCapture())
        {
            Add1Sub2(prev_acc, next_acc, { w_king, b_king, to_sq, from_piece }, { w_king, b_king, from_sq, from_piece },
                { w_king, b_king, to_sq, cap_piece });
        }
        else
        {
            Add1Sub1(
                prev_acc, next_acc, { w_king, b_king, to_sq, from_piece }, { w_king, b_king, from_sq, from_piece });
        }
    }
    else
    {
        if (move.IsPromotion() && move.IsCapture())
        {
            Add1Sub2(prev_acc, next_acc, { w_king, b_king, to_sq, to_piece }, { w_king, b_king, from_sq, from_piece },
                { w_king, b_king, to_sq, cap_piece });
        }
        else if (move.IsPromotion())
        {
            Add1Sub1(prev_acc, next_acc, { w_king, b_king, to_sq, to_piece }, { w_king, b_king, from_sq, from_piece });
        }
        else if (move.IsCapture() && move.GetFlag() == EN_PASSANT)
        {
            auto ep_capture_sq = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));
            Add1Sub2(prev_acc, next_acc, { w_king, b_king, to_sq, from_piece }, { w_king, b_king, from_sq, from_piece },
                { w_king, b_king, ep_capture_sq, Piece(PAWN, !stm) });
        }
        else if (move.IsCapture())
        {
            Add1Sub2(prev_acc, next_acc, { w_king, b_king, to_sq, from_piece }, { w_king, b_king, from_sq, from_piece },
                { w_king, b_king, to_sq, cap_piece });
        }
        else
        {
            Add1Sub1(
                prev_acc, next_acc, { w_king, b_king, to_sq, from_piece }, { w_king, b_king, from_sq, from_piece });
        }
    }

    assert(Verify(post_move_board, next_acc));
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

Score Network::Eval(const BoardState& board, const Accumulator& acc)
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(GetBitCount(board.GetAllPieces()));

    int32_t output = net.outputBias[output_bucket];
    DotProductHalves(SCReLU(acc.side[stm]), SCReLU(acc.side[!stm]), net.outputWeights[output_bucket], output);
    output *= SCALE_FACTOR;
    output /= L1_SCALE * L2_SCALE;
    return output;
}

Score Network::SlowEval(const BoardState& board)
{
    Accumulator acc;
    acc.Recalculate(board);
    return Eval(board, acc);
}
