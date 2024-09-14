#include "Network.h"

#include <algorithm>
#include <cstdint>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "incbin/incbin.h"

#undef INCBIN_ALIGNMENT
#define INCBIN_ALIGNMENT 64
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

constexpr int16_t L1_SCALE = 255;
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
void DotProductSCReLU(const std::array<T_in, SIZE>& stm, const std::array<T_in, SIZE>& other,
    const std::array<T_in, SIZE * 2>& weights, T_out& output)
{
    // This uses the lizard-SCReLU trick: Given stm[i] < 255, and weights[i] < 126, we want to compute stm[i] * stm[i] *
    // weights[i] to do the SCReLU dot product. By first doing stm[i] * weights[i], the result fits within the int16_t
    // type. Then multiply by stm[i] and accumulate.

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t partial = stm[i] * weights[i];
        output += partial * stm[i];
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        int16_t partial = other[i] * weights[i + SIZE];
        output += partial * other[i];
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

void Add1Sub1(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1, Players view)
{
    size_t add1_index = index(add1.king[view], add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king[view], sub1.piece_sq, sub1.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j];
    }
}

void Add1Sub2(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1,
    const InputPair& sub2, Players view)
{
    size_t add1_index = index(add1.king[view], add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king[view], sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king[view], sub2.piece_sq, sub2.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j]
            - net.hiddenWeights[sub2_index][j];
    }
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& add2,
    const InputPair& sub1, const InputPair& sub2, Players view)
{
    size_t add1_index = index(add1.king[view], add1.piece_sq, add1.piece, view);
    size_t add2_index = index(add2.king[view], add2.piece_sq, add2.piece, view);
    size_t sub1_index = index(sub1.king[view], sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king[view], sub2.piece_sq, sub2.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        next.side[view][j] = prev.side[view][j] + net.hiddenWeights[add1_index][j] - net.hiddenWeights[sub1_index][j]
            + net.hiddenWeights[add2_index][j] - net.hiddenWeights[sub2_index][j];
    }
}

void Accumulator::AddInput(const InputPair& input)
{
    AddInput({ input.king[WHITE], input.piece_sq, input.piece }, WHITE);
    AddInput({ input.king[BLACK], input.piece_sq, input.piece }, BLACK);
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
    SubInput({ input.king[WHITE], input.piece_sq, input.piece }, WHITE);
    SubInput({ input.king[BLACK], input.piece_sq, input.piece }, BLACK);
}

void Accumulator::SubInput(const Input& input, Players view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);

    for (size_t j = 0; j < HIDDEN_NEURONS; j++)
    {
        side[view][j] -= net.hiddenWeights[side_index][j];
    }
}

void Accumulator::Recalculate(const BoardState& board_)
{
    Recalculate(board_, WHITE);
    Recalculate(board_, BLACK);
    acc_is_valid = { true, true };
}

void Accumulator::Recalculate(const BoardState& board_, Players view)
{
    side[view] = net.hiddenBias;
    auto king = board_.GetKing(view);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board_.GetPieceBB(piece);

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
        entry.acc = {};
        entry.acc.side = { net.hiddenBias, net.hiddenBias };
        entry.white_bb = {};
        entry.black_bb = {};
    }
}

bool Network::Verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = {};
    expected.side = { net.hiddenBias, net.hiddenBias };
    std::array<Square, N_PLAYERS> king;
    king[WHITE] = board.GetKing(WHITE);
    king[BLACK] = board.GetKing(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Pieces piece = static_cast<Pieces>(i);
        uint64_t bb = board.GetPieceBB(piece);

        while (bb)
        {
            Square sq = LSBpop(bb);
            expected.AddInput({ king, sq, piece });
        }
    }

    return expected == acc;
}

void Network::StoreLazyUpdates(
    const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move)
{
    acc.acc_is_valid = { false, false };
    acc.requires_recalculation = { false, false };
    acc.adds = {};
    acc.n_adds = 0;
    acc.subs = {};
    acc.n_subs = 0;
    acc.board = {};

    if (move == Move::Uninitialized)
    {
        return;
    }

    auto stm = prev_move_board.stm;
    auto from_sq = move.GetFrom();
    auto to_sq = move.GetTo();
    auto from_piece = prev_move_board.GetSquare(from_sq);
    auto to_piece = post_move_board.GetSquare(to_sq);
    auto cap_piece = prev_move_board.GetSquare(to_sq);

    std::array<Square, N_PLAYERS> king;
    king[WHITE] = post_move_board.GetKing(WHITE);
    king[BLACK] = post_move_board.GetKing(BLACK);

    if (from_piece == Piece(KING, stm))
    {
        if (get_king_bucket(from_sq, stm) != get_king_bucket(to_sq, stm)
            || ((GetFile(from_sq) <= FILE_D) ^ (GetFile(to_sq) <= FILE_D)))
        {
            acc.requires_recalculation[stm] = true;
            acc.board = post_move_board;

            if (move.IsCastle())
            {
                Square king_start = move.GetFrom();
                Square king_end
                    = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
                Square rook_start = move.GetTo();
                Square rook_end
                    = move.GetFlag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

                acc.n_adds = 2;
                acc.n_subs = 2;

                acc.adds[0] = { king, king_end, from_piece };
                acc.adds[1] = { king, rook_end, cap_piece };

                acc.subs[0] = { king, king_start, from_piece };
                acc.subs[1] = { king, rook_start, cap_piece };
            }
            else if (move.IsCapture())
            {
                acc.n_adds = 1;
                acc.n_subs = 2;

                acc.adds[0] = { king, to_sq, from_piece };

                acc.subs[0] = { king, from_sq, from_piece };
                acc.subs[1] = { king, to_sq, cap_piece };
            }
            else
            {
                acc.n_adds = 1;
                acc.n_subs = 1;

                acc.adds[0] = { king, to_sq, from_piece };

                acc.subs[0] = { king, from_sq, from_piece };
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

            acc.n_adds = 2;
            acc.n_subs = 2;

            acc.adds[0] = { king, king_end, from_piece };
            acc.adds[1] = { king, rook_end, cap_piece };

            acc.subs[0] = { king, king_start, from_piece };
            acc.subs[1] = { king, rook_start, cap_piece };
        }
        else if (move.IsCapture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { king, to_sq, from_piece };

            acc.subs[0] = { king, from_sq, from_piece };
            acc.subs[1] = { king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { king, to_sq, from_piece };

            acc.subs[0] = { king, from_sq, from_piece };
        }
    }
    else
    {
        if (move.IsPromotion() && move.IsCapture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { king, to_sq, to_piece };

            acc.subs[0] = { king, from_sq, from_piece };
            acc.subs[1] = { king, to_sq, cap_piece };
        }
        else if (move.IsPromotion())
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { king, to_sq, to_piece };

            acc.subs[0] = { king, from_sq, from_piece };
        }
        else if (move.IsCapture() && move.GetFlag() == EN_PASSANT)
        {
            auto ep_capture_sq = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));

            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { king, to_sq, from_piece };

            acc.subs[0] = { king, from_sq, from_piece };
            acc.subs[1] = { king, ep_capture_sq, Piece(PAWN, !stm) };
        }
        else if (move.IsCapture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { king, to_sq, from_piece };

            acc.subs[0] = { king, from_sq, from_piece };
            acc.subs[1] = { king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { king, to_sq, from_piece };

            acc.subs[0] = { king, from_sq, from_piece };
        }
    }
}

void Network::ApplyLazyUpdates(const Accumulator& prev_acc, Accumulator& next_acc, Players view)
{
    if (next_acc.acc_is_valid[view])
    {
        return;
    }

    assert(!(next_acc.requires_recalculation[WHITE] && next_acc.requires_recalculation[BLACK]));

    if (next_acc.requires_recalculation[view])
    {
        table.Recalculate(next_acc, next_acc.board, view, next_acc.board.GetKing(view));
    }
    else
    {
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], view);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1], view);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1], view);
        }
        else if (next_acc.n_adds == 0 && next_acc.n_subs == 0)
        {
            // null move
            next_acc.side[view] = prev_acc.side[view];
        }
    }

    next_acc.acc_is_valid[view] = true;
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

Score Network::Eval(const BoardState& board, const Accumulator& acc)
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(GetBitCount(board.GetAllPieces()));

    int32_t output = 0;
    DotProductSCReLU(CReLU(acc.side[stm]), CReLU(acc.side[!stm]), net.outputWeights[output_bucket], output);
    return (int64_t(output) + int64_t(net.outputBias[output_bucket]) * L1_SCALE) * SCALE_FACTOR / L1_SCALE / L1_SCALE
        / L2_SCALE;
}

Score Network::SlowEval(const BoardState& board)
{
    Accumulator acc;
    acc.Recalculate(board);
    return Eval(board, acc);
}
