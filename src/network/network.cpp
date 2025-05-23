#include "network.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "network/accumulator.hpp"
#include "network/inference.hpp"
#include "nn/Arch.hpp"
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
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
} const& raw_net = reinterpret_cast<const raw_network&>(*gNetData);

[[maybe_unused]] auto verify_network_size = []
{
    assert(sizeof(raw_network) == gNetSize);
    return true;
}();

auto adjust_for_packus(const decltype(raw_network::l1_weight)& input)
{
    auto output = std::make_unique<decltype(raw_network::l1_weight)>();

#if defined(USE_AVX2)
    // shuffle around l1 weights to match packus interleaving
    for (size_t i = 0; i < raw_net.l1_weight.size(); i++)
    {
        for (size_t j = 0; j < raw_net.l1_weight[i].size(); j++)
        {
            for (size_t k = 0; k < raw_net.l1_weight[i][j].size(); k += sizeof(SIMD::vec) / sizeof(int8_t))
            {
#if defined(USE_AVX512)
                constexpr std::array mapping = { 0, 4, 1, 5, 2, 6, 3, 7 };
#else
                constexpr std::array mapping = { 0, 2, 1, 3 };
#endif
                for (size_t x = 0; x < sizeof(SIMD::vec) / sizeof(int8_t); x++)
                {
                    (*output)[i][j][k + x] = input[i][j][k + mapping[x / 8] * 8 + x % 8];
                }
            }
        }
    }
#else
    (*output) = input;
#endif
    return output;
}

auto rescale_l1_bias(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<decltype(raw_network::l1_bias)>();

    // rescale l1 bias to account for FT_activation adjusting to 0-127 scale
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = input[i][j] * 127 / FT_SCALE;
        }
    }

    return output;
}

auto interleave_for_l1_sparsity(const decltype(raw_network::l1_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

#if defined(SIMD_ENABLED)
    // transpose and interleave the weights in blocks of 4 FT activations
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                // we want something that looks like:
                //[bucket][nibble][output][4]
                (*output)[i][(k / 4) * (4 * L1_SIZE) + (j * 4) + (k % 4)] = input[i][j][k];
            }
        }
    }
#else
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                (*output)[i][j * FT_SIZE + k] = input[i][j][k];
            }
        }
    }
#endif
    return output;
}

auto cast_l1_weight_int8(const std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>& input)
{
    auto output = std::make_unique<std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < FT_SIZE * L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int8_t>(input[i][j]);
        }
    }

    return output;
}

auto cast_l1_bias_int32(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int32_t>(input[i][j]);
        }
    }

    return output;
}

auto shuffle_ft_neurons(const decltype(raw_network::ft_weight)& ft_weight,
    const decltype(raw_network::ft_bias)& ft_bias, const decltype(raw_network::l1_weight)& l1_weight)
{
    // Based on some heuristic, we want to sort the FT neurons to maximize the benefit of the l1 sparsity.

    // clang-format off
    [[maybe_unused]] constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 413, 285, 429, 127, 234, 104, 16, 14,
        214, 235, 328, 380, 90, 65, 88, 399, 424, 398, 389, 392, 354, 355, 28, 314, 417, 240, 250, 368, 286, 251, 189,
        443, 174, 247, 319, 276, 448, 397, 357, 366, 236, 358, 306, 498, 493, 2, 148, 488, 71, 439, 406, 20, 302, 164,
        363, 39, 220, 263, 462, 432, 1, 139, 401, 226, 21, 160, 415, 69, 479, 141, 30, 270, 186, 304, 331, 434, 291,
        309, 68, 486, 396, 425, 273, 185, 468, 338, 308, 13, 183, 77, 118, 42, 371, 267, 175, 353, 388, 177, 18, 93,
        121, 503, 239, 289, 79, 269, 343, 147, 191, 38, 386, 51, 142, 162, 451, 501, 351, 497, 58, 411, 473, 326, 9,
        502, 0, 261, 307, 374, 387, 223, 483, 83, 268, 178, 477, 435, 116, 203, 280, 19, 109, 323, 115, 126, 258, 315,
        255, 407, 508, 257, 145, 455, 459, 230, 143, 34, 476, 92, 295, 381, 444, 102, 221, 17, 505, 81, 37, 329, 31, 73,
        157, 487, 152, 131, 149, 350, 205, 153, 206, 442, 409, 334, 305, 485, 100, 176, 199, 412, 382, 25, 181, 377,
        457, 266, 342, 5, 129, 252, 422, 489, 408, 198, 74, 225, 322, 3, 446, 101, 167, 395, 490, 200, 155, 404, 82,
        367, 210, 478, 204, 420, 215, 456, 299, 298, 84, 324, 344, 212, 265, 161, 430, 400, 53, 144, 228, 480, 313, 45,
        369, 256, 372, 390, 201, 244, 370, 303, 452, 85, 180, 356, 272, 391, 378, 458, 274, 80, 173, 290, 482, 310, 327,
        492, 321, 504, 113, 332, 242, 196, 359, 67, 238, 494, 156, 26, 193, 427, 172, 474, 146, 163, 171, 182, 213, 506,
        46, 433, 246, 348, 402, 277, 275, 279, 405, 237, 301, 29, 376, 41, 453, 336, 60, 447, 169, 12, 346, 194, 44,
        166, 262, 195, 248, 91, 373, 491, 32, 8, 454, 379, 495, 449, 423, 333, 150, 393, 97, 54, 418, 170, 511, 219,
        253, 151, 154, 75, 254, 231, 340, 50, 192, 179, 27, 188, 232, 428, 437, 463, 123, 4, 105, 61, 99, 132, 339, 260,
        159, 76, 283, 207, 471, 59, 509, 292, 484, 450, 86, 325, 352, 117, 470, 341, 120, 108, 467, 460, 300, 89, 229,
        500, 385, 431, 410, 52, 103, 130, 441, 48, 133, 496, 318, 63, 296, 49, 445, 233, 22, 70, 507, 241, 137, 294, 43,
        222, 282, 466, 287, 111, 35, 87, 320, 361, 128, 33, 375, 134, 349, 110, 347, 202, 249, 245, 217, 281, 40, 165,
        465, 394, 278, 414, 218, 190, 23, 224, 312, 293, 426, 15, 436, 94, 64, 107, 72, 384, 216, 98, 184, 335, 187,
        140, 365, 227, 197, 66, 211, 62, 416, 136, 464, 112, 383, 124, 362, 135, 403, 360, 316, 11, 47, 57, 317, 114,
        119, 481, 472, 499, 345, 311, 7, 271, 337, 421, 330, 106, 56, 364, 10, 440, 158, 209, 510, 95, 259, 24, 6, 122,
        208, 419, 438, 125, 96, 297, 55, 475, 168, 36, 284, 288, 78, 264, 243, 469, 461, 138,
    };
    // clang-format on

    auto ft_weight_output = std::make_unique<decltype(raw_network::ft_weight)>();
    auto ft_bias_output = std::make_unique<decltype(raw_network::ft_bias)>();
    auto l1_weight_output = std::make_unique<decltype(raw_network::l1_weight)>();

    for (size_t i = 0; i < FT_SIZE / 2; i++)
    {
        auto old_idx = shuffle_order[i];

        (*ft_bias_output)[i] = ft_bias[old_idx];
        (*ft_bias_output)[i + FT_SIZE / 2] = ft_bias[old_idx + FT_SIZE / 2];

        for (size_t j = 0; j < INPUT_SIZE * KING_BUCKET_COUNT; j++)
        {
            (*ft_weight_output)[j][i] = ft_weight[j][old_idx];
            (*ft_weight_output)[j][i + FT_SIZE / 2] = ft_weight[j][old_idx + FT_SIZE / 2];
        }

        for (size_t j = 0; j < OUTPUT_BUCKETS; j++)
        {
            for (size_t k = 0; k < L1_SIZE; k++)
            {
                (*l1_weight_output)[j][k][i] = l1_weight[j][k][old_idx];
                (*l1_weight_output)[j][k][i + FT_SIZE / 2] = l1_weight[j][k][old_idx + FT_SIZE / 2];
            }
        }
    }

    return std::make_tuple(std::move(ft_weight_output), std::move(ft_bias_output), std::move(l1_weight_output));
}

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
} const& net = []
{
    auto [ft_weight, ft_bias, l1_weight] = shuffle_ft_neurons(raw_net.ft_weight, raw_net.ft_bias, raw_net.l1_weight);

    auto l1_weight_2 = adjust_for_packus(*l1_weight);
    auto l1_bias = rescale_l1_bias(raw_net.l1_bias);
    auto l1_weight_3 = interleave_for_l1_sparsity(*l1_weight_2);

    auto final_net = std::make_unique<network>();
    final_net->ft_weight = *ft_weight;
    final_net->ft_bias = *ft_bias;
    final_net->l1_weight = *cast_l1_weight_int8(*l1_weight_3);
    final_net->l1_bias = *cast_l1_bias_int32(*l1_bias);
    final_net->l2_weight = raw_net.l2_weight;
    final_net->l2_bias = raw_net.l2_bias;
    final_net->l3_weight = raw_net.l3_weight;
    final_net->l3_bias = raw_net.l3_bias;
    return *final_net;
}();
constexpr int16_t L2_SCALE = 64;
constexpr double SCALE_FACTOR = 160;

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

    alignas(64) std::array<uint8_t, FT_SIZE> ft_activation;
    alignas(64) std::array<int16_t, FT_SIZE / 4> sparse_ft_nibbles;
    size_t sparse_nibbles_size = 0;
    NN::Features::FT_activation(acc.side[stm], acc.side[!stm], ft_activation, sparse_ft_nibbles, sparse_nibbles_size);
    assert(std::all_of(ft_activation.begin(), ft_activation.end(), [](auto x) { return x <= 127; }));

    alignas(64) std::array<int32_t, L1_SIZE> l1_activation = net.l1_bias[output_bucket];
    NN::Features::L1_activation(
        ft_activation, net.l1_weight[output_bucket], sparse_ft_nibbles, sparse_nibbles_size, l1_activation);
    assert(
        std::all_of(l1_activation.begin(), l1_activation.end(), [](auto x) { return 0 <= x && x <= 127 * L1_SCALE; }));

    alignas(64) std::array<float, L2_SIZE> l2_activation = net.l2_bias[output_bucket];
    NN::Features::L2_activation(l1_activation, net.l2_weight[output_bucket], l2_activation);
    assert(std::all_of(l2_activation.begin(), l2_activation.end(), [](auto x) { return 0 <= x && x <= 1; }));

    float output = net.l3_bias[output_bucket];
    NN::Features::L3_activation(l2_activation, net.l3_weight[output_bucket], output);

    return output * SCALE_FACTOR;
}

Score Network::slow_eval(const BoardState& board)
{
    Accumulator acc;
    acc.recalculate(board);
    return eval(board, acc);
}

}