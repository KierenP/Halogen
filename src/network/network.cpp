#include "network.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "network/accumulator.hpp"
#include "network/inference.hpp"
#include "third-party/incbin/incbin.h"

namespace NN
{

#undef INCBIN_ALIGNMENT
#define INCBIN_ALIGNMENT 64
INCBIN(Net, EVALFILE);

struct raw_network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE * 2>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> out_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> out_bias = {};
} const& raw_net = reinterpret_cast<const raw_network&>(*gNetData);

[[maybe_unused]] auto verify_network_size = []
{
    assert(sizeof(raw_network) == gNetSize);
    return true;
}();

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int8_t, FT_SIZE * 2>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> out_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> out_bias = {};
} const& net = []
{
    network shuffled_net;
    shuffled_net.ft_weight = raw_net.ft_weight;
    shuffled_net.ft_bias = raw_net.ft_bias;

#if defined(USE_AVX2)

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE * 2; k += sizeof(SIMD::vec) / sizeof(int8_t))
            {
#if defined(USE_AVX512)
                constexpr std::array mapping = { 0, 4, 1, 5, 2, 6, 3, 7 };
#else
                constexpr std::array mapping = { 0, 2, 1, 3 };
#endif
                for (size_t x = 0; x < sizeof(SIMD::vec) / sizeof(int8_t); x++)
                {
                    shuffled_net.l1_weight[i][j][k + x]
                        = static_cast<int8_t>(raw_net.l1_weight[i][j][k + mapping[x / 8] * 8 + x % 8]);
                }
            }
        }
    }

#else
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE * 2; k++)
            {
                shuffled_net.l1_weight[i][j][k] = static_cast<int8_t>(raw_net.l1_weight[i][j][k]);
            }
        }
    }

#endif

    // l1 activations are stores as int32_t, so it makes it easier if the bias is also int32_t
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            shuffled_net.l1_bias[i][j] = raw_net.l1_bias[i][j];
        }
    }

    shuffled_net.l2_weight = raw_net.l2_weight;
    shuffled_net.l2_bias = raw_net.l2_bias;
    shuffled_net.out_weight = raw_net.out_weight;
    shuffled_net.out_bias = raw_net.out_bias;
    return shuffled_net;
}();
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

template <typename T, size_t SIZE, T SCALE>
[[nodiscard]] std::array<T, SIZE> CReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
        ret[i] = std::clamp(source[i], T(0), SCALE);

    return ret;
}

template <typename T, size_t SIZE, T SCALE>
[[nodiscard]] std::array<T, SIZE> SCReLU(const std::array<T, SIZE>& source)
{
    std::array<T, SIZE> ret;

    for (size_t i = 0; i < SIZE; i++)
    {
        ret[i] = std::clamp(source[i], T(0), SCALE);
        ret[i] = ret[i] * ret[i] / SCALE;
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
        T_in partial = stm[i] * weights[i];
        output += partial * stm[i];
    }

    for (size_t i = 0; i < SIZE; i++)
    {
        T_in partial = other[i] * weights[i + SIZE];
        output += partial * other[i];
    }
}

template <typename T_out, typename T1_in, typename T2_in, size_t SIZE>
void DotProduct(const std::array<T1_in, SIZE>& a, const std::array<T2_in, SIZE>& b, T_out& output)
{
    for (size_t i = 0; i < SIZE; i++)
    {
        output += a[i] * b[i];
    }
}

int get_king_bucket(Square king_sq, Side view)
{
    king_sq = view == WHITE ? king_sq : flip_square_vertical(king_sq);
    return KING_BUCKETS[king_sq];
}

int index(Square king_sq, Square piece_sq, Piece piece, Side view)
{
    piece_sq = view == WHITE ? piece_sq : flip_square_vertical(piece_sq);
    piece_sq = enum_to<File>(king_sq) <= FILE_D ? piece_sq : flip_square_horizontal(piece_sq);

    auto king_bucket = get_king_bucket(king_sq, view);
    Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
    PieceType pieceType = enum_to<PieceType>(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const Input& add1, const Input& sub1, Side view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);

    add1sub1(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[sub1_index]);
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1)
{
    Add1Sub1(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { sub1.w_king, sub1.piece_sq, sub1.piece }, WHITE);
    Add1Sub1(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { sub1.b_king, sub1.piece_sq, sub1.piece }, BLACK);
}

void Add1Sub2(
    const Accumulator& prev, Accumulator& next, const Input& add1, const Input& sub1, const Input& sub2, Side view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king_sq, sub2.piece_sq, sub2.piece, view);

    add1sub2(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[sub1_index],
        net.ft_weight[sub2_index]);
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
    const Input& sub2, Side view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t add2_index = index(add2.king_sq, add2.piece_sq, add2.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king_sq, sub2.piece_sq, sub2.piece, view);

    add2sub2(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[add2_index],
        net.ft_weight[sub1_index], net.ft_weight[sub2_index]);
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& add2,
    const InputPair& sub1, const InputPair& sub2)
{
    Add2Sub2(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { add2.w_king, add2.piece_sq, add2.piece },
        { sub1.w_king, sub1.piece_sq, sub1.piece }, { sub2.w_king, sub2.piece_sq, sub2.piece }, WHITE);
    Add2Sub2(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { add2.b_king, add2.piece_sq, add2.piece },
        { sub1.b_king, sub1.piece_sq, sub1.piece }, { sub2.b_king, sub2.piece_sq, sub2.piece }, BLACK);
}

void Accumulator::add_input(const InputPair& input)
{
    add_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    add_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::add_input(const Input& input, Side view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);
    add1(side[view], net.ft_weight[side_index]);
}

void Accumulator::sub_input(const InputPair& input)
{
    sub_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    sub_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::sub_input(const Input& input, Side view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);
    sub1(side[view], net.ft_weight[side_index]);
}

void Accumulator::recalculate(const BoardState& board_)
{
    recalculate(board_, WHITE);
    recalculate(board_, BLACK);
    acc_is_valid = true;
}

void Accumulator::recalculate(const BoardState& board_, Side view)
{
    side[view] = net.ft_bias;
    auto king = board_.GetKing(view);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board_.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            add_input({ king, sq, piece }, view);
        }
    }
}

void AccumulatorTable::recalculate(Accumulator& acc, const BoardState& board, Side side, Square king_sq)
{
    // we need to keep a separate entry for when the king is on each side of the board
    auto& entry
        = king_bucket[get_king_bucket(king_sq, side) + (enum_to<File>(king_sq) <= FILE_D ? 0 : KING_BUCKET_COUNT)];
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
        auto new_bb = board.get_pieces_bb(piece);
        auto& old_bb = bb[piece];

        auto to_add = new_bb & ~old_bb;
        auto to_sub = old_bb & ~new_bb;

        while (to_add)
        {
            auto sq = lsbpop(to_add);
            entry.acc.add_input({ king_sq, sq, piece }, side);
        }

        while (to_sub)
        {
            auto sq = lsbpop(to_sub);
            entry.acc.sub_input({ king_sq, sq, piece }, side);
        }

        old_bb = new_bb;
    }

    acc.side[side] = entry.acc.side[side];
}

void Network::reset_new_search(const BoardState& board, Accumulator& acc)
{
    acc.recalculate(board);

    for (auto& entry : table.king_bucket)
    {
        entry.acc = {};
        entry.acc.side = { net.ft_bias, net.ft_bias };
        entry.white_bb = {};
        entry.black_bb = {};
    }
}

bool Network::verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = {};
    expected.side = { net.ft_bias, net.ft_bias };
    auto w_king = board.get_king_sq(WHITE);
    auto b_king = board.get_king_sq(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            expected.add_input({ w_king, b_king, sq, piece });
        }
    }

    return expected == acc;
}

void Network::store_lazy_updates(
    const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move)
{
    acc.acc_is_valid = false;
    acc.white_requires_recalculation = false;
    acc.black_requires_recalculation = false;
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
    auto from_sq = move.from();
    auto to_sq = move.to();
    auto from_piece = prev_move_board.get_square_piece(from_sq);
    auto to_piece = post_move_board.get_square_piece(to_sq);
    auto cap_piece = prev_move_board.get_square_piece(to_sq);

    auto w_king = post_move_board.get_king_sq(WHITE);
    auto b_king = post_move_board.get_king_sq(BLACK);

    if (from_piece == get_piece(KING, stm))
    {
        if (get_king_bucket(from_sq, stm) != get_king_bucket(to_sq, stm)
            || ((enum_to<File>(from_sq) <= FILE_D) ^ (enum_to<File>(to_sq) <= FILE_D)))
        {
            if (stm == WHITE)
            {
                acc.white_requires_recalculation = true;
            }
            else
            {
                acc.black_requires_recalculation = true;
            }

            acc.board = post_move_board;

            if (move.is_castle())
            {
                Square king_start = move.from();
                Square king_end
                    = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
                Square rook_start = move.to();
                Square rook_end
                    = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

                acc.n_adds = 2;
                acc.n_subs = 2;

                acc.adds[0] = { w_king, b_king, king_end, from_piece };
                acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

                acc.subs[0] = { w_king, b_king, king_start, from_piece };
                acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
            }
            else if (move.is_capture())
            {
                acc.n_adds = 1;
                acc.n_subs = 2;

                acc.adds[0] = { w_king, b_king, to_sq, from_piece };

                acc.subs[0] = { w_king, b_king, from_sq, from_piece };
                acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
            }
            else
            {
                acc.n_adds = 1;
                acc.n_subs = 1;

                acc.adds[0] = { w_king, b_king, to_sq, from_piece };

                acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            }
        }
        else if (move.is_castle())
        {
            Square king_start = move.from();
            Square king_end
                = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
            Square rook_start = move.to();
            Square rook_end
                = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

            acc.n_adds = 2;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, king_end, from_piece };
            acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

            acc.subs[0] = { w_king, b_king, king_start, from_piece };
            acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
        }
        else if (move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
    }
    else
    {
        if (move.is_promotion() && move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else if (move.is_promotion())
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
        else if (move.is_capture() && move.flag() == EN_PASSANT)
        {
            auto ep_capture_sq = get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()));

            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, ep_capture_sq, get_piece(PAWN, !stm) };
        }
        else if (move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
    }
}

void Network::apply_lazy_updates(const Accumulator& prev_acc, Accumulator& next_acc)
{
    if (next_acc.acc_is_valid)
    {
        return;
    }

    assert(!(next_acc.white_requires_recalculation && next_acc.black_requires_recalculation));

    if (next_acc.white_requires_recalculation)
    {
        table.recalculate(next_acc, next_acc.board, WHITE, next_acc.board.get_king_sq(WHITE));

        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece }, BLACK);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].b_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, BLACK);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.adds[1].b_king, next_acc.adds[1].piece_sq, next_acc.adds[1].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].b_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, BLACK);
        }
    }
    else if (next_acc.black_requires_recalculation)
    {
        table.recalculate(next_acc, next_acc.board, BLACK, next_acc.board.get_king_sq(BLACK));

        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece }, WHITE);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].w_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, WHITE);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.adds[1].w_king, next_acc.adds[1].piece_sq, next_acc.adds[1].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].w_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, WHITE);
        }
    }
    else
    {
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0]);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 0 && next_acc.n_subs == 0)
        {
            // null move
            next_acc.side = prev_acc.side;
        }
    }

    next_acc.acc_is_valid = true;
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

Score Network::eval(const BoardState& board, const Accumulator& acc)
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(popcount(board.get_pieces_bb()));

    alignas(64) std::array<uint8_t, FT_SIZE * 2> ft_activation;
    NN::Features::FT_activation(acc.side[stm], acc.side[!stm], ft_activation);

    assert(std::all_of(ft_activation.begin(), ft_activation.end(), [](auto x) { return x <= 127; }));

    alignas(64) std::array<int32_t, L1_SIZE> l1_activation = net.l1_bias[output_bucket];
    NN::Features::L1_activation(ft_activation, net.l1_weight[output_bucket], l1_activation);

    assert(
        std::all_of(l1_activation.begin(), l1_activation.end(), [](auto x) { return 0 <= x && x <= 127 * L1_SCALE; }));

    // from here on, we use float activations
    // TODO: SIMD
    std::array<float, L1_SIZE> l1_float;

    for (size_t i = 0; i < L1_SIZE; i++)
    {
        // 127 to match FT_activation adjustment
        l1_float[i] = float(l1_activation[i]) / (127 * L1_SCALE);
    }

    assert(std::all_of(l1_float.begin(), l1_float.end(), [](auto x) { return 0 <= x && x <= 1; }));

    std::array<float, L2_SIZE> l2 = net.l2_bias[output_bucket];

    for (size_t i = 0; i < L2_SIZE; i++)
    {
        DotProduct(l1_float, net.l2_weight[output_bucket][i], l2[i]);
    }

    l2 = CReLU<float, L2_SIZE, 1.f>(l2);

    float output = net.out_bias[output_bucket];
    DotProduct(l2, net.out_weight[output_bucket], output);

    return output * SCALE_FACTOR;
}

Score Network::slow_eval(const BoardState& board)
{
    Accumulator acc;
    acc.recalculate(board);
    return eval(board, acc);
}

}