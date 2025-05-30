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

#include "network/Features.hpp"
#include "simd/Definitions.hpp"

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
            for (size_t k = 0; k < raw_net.l1_weight[i][j].size(); k += SIMD::vec_size)
            {
#if defined(USE_AVX512)
                constexpr std::array mapping = { 0, 4, 1, 5, 2, 6, 3, 7 };
#else
                constexpr std::array mapping = { 0, 2, 1, 3 };
#endif
                for (size_t x = 0; x < SIMD::vec_size; x++)
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
    auto ft_weight_output = std::make_unique<decltype(raw_network::ft_weight)>();
    auto ft_bias_output = std::make_unique<decltype(raw_network::ft_bias)>();
    auto l1_weight_output = std::make_unique<decltype(raw_network::l1_weight)>();

#ifdef NETWORK_SHUFFLE
    (*ft_weight_output) = ft_weight;
    (*ft_bias_output) = ft_bias;
    (*l1_weight_output) = l1_weight;
#else
    // Based on some heuristic, we want to sort the FT neurons to maximize the benefit of the l1 sparsity.

    // clang-format off
    [[maybe_unused]] constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 413, 429, 285, 127, 235, 104, 234,
        328, 14, 88, 65, 214, 380, 424, 90, 389, 16, 398, 392, 399, 354, 240, 174, 28, 355, 251, 357, 417, 358, 443,
        189, 276, 368, 250, 314, 319, 247, 366, 448, 286, 488, 164, 148, 439, 397, 71, 236, 306, 2, 498, 406, 263, 493,
        302, 462, 30, 141, 401, 432, 309, 374, 477, 121, 17, 39, 363, 139, 415, 20, 331, 160, 21, 220, 177, 13, 185, 69,
        304, 226, 1, 479, 502, 308, 9, 396, 77, 371, 79, 270, 269, 118, 338, 291, 468, 142, 147, 434, 486, 38, 255, 451,
        223, 58, 326, 386, 267, 68, 51, 273, 175, 353, 42, 93, 388, 425, 83, 503, 289, 162, 387, 261, 19, 183, 186, 18,
        239, 351, 109, 404, 307, 381, 422, 126, 203, 411, 483, 435, 350, 377, 343, 280, 152, 81, 181, 501, 149, 102,
        473, 178, 409, 446, 198, 497, 129, 508, 205, 257, 485, 329, 143, 459, 266, 193, 457, 25, 407, 221, 191, 315,
        305, 74, 356, 295, 3, 213, 144, 298, 173, 505, 100, 115, 299, 382, 230, 116, 444, 0, 476, 73, 412, 370, 489,
        323, 131, 244, 342, 157, 167, 395, 153, 268, 176, 101, 480, 400, 290, 428, 228, 456, 408, 478, 155, 34, 334, 82,
        188, 8, 146, 171, 420, 378, 91, 37, 455, 246, 327, 303, 159, 367, 201, 348, 487, 31, 225, 369, 85, 310, 195,
        471, 156, 252, 180, 494, 46, 210, 53, 161, 265, 490, 372, 199, 206, 170, 275, 113, 402, 390, 258, 145, 212, 442,
        200, 324, 92, 384, 260, 312, 383, 293, 256, 430, 41, 492, 215, 274, 346, 405, 376, 137, 70, 29, 123, 491, 154,
        80, 292, 320, 35, 238, 437, 339, 496, 359, 63, 150, 321, 474, 344, 283, 237, 504, 132, 32, 196, 5, 166, 427, 54,
        262, 76, 277, 509, 84, 67, 97, 458, 391, 379, 204, 279, 194, 182, 272, 332, 27, 340, 44, 506, 465, 107, 89, 6,
        313, 12, 242, 482, 450, 229, 423, 134, 453, 75, 128, 318, 454, 248, 325, 333, 26, 179, 120, 108, 57, 48, 385,
        278, 393, 322, 441, 86, 463, 349, 207, 249, 4, 163, 60, 33, 362, 119, 233, 294, 500, 414, 61, 23, 301, 15, 418,
        45, 452, 495, 169, 151, 105, 40, 231, 254, 352, 197, 158, 365, 445, 511, 433, 431, 373, 99, 103, 219, 336, 130,
        499, 361, 375, 470, 165, 112, 341, 172, 259, 184, 66, 94, 7, 187, 117, 124, 507, 484, 282, 245, 426, 202, 449,
        192, 72, 133, 22, 224, 114, 317, 296, 311, 241, 316, 52, 466, 447, 364, 394, 467, 49, 300, 481, 360, 330, 335,
        436, 43, 50, 232, 253, 110, 217, 464, 410, 98, 211, 421, 460, 190, 271, 287, 243, 24, 208, 264, 347, 440, 416,
        403, 227, 209, 64, 469, 62, 10, 216, 135, 510, 337, 59, 438, 11, 345, 122, 56, 461, 47, 95, 472, 222, 281, 87,
        96, 111, 218, 106, 419, 36, 78, 284, 125, 136, 140, 288, 297, 55, 475, 168, 138,
    };
    // clang-format on

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
#endif
    return std::make_tuple(std::move(ft_weight_output), std::move(ft_bias_output), std::move(l1_weight_output));
}

auto transpose_l2_weights(const decltype(raw_net.l2_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < L2_SIZE; k++)
            {
                (*output)[i][j][k] = input[i][k][j];
            }
        }
    }

    return output;
}

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
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
    final_net->l2_weight = *transpose_l2_weights(raw_net.l2_weight);
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

    alignas(64) std::array<float, L1_SIZE> l1_activation;
    NN::Features::L1_activation(ft_activation, net.l1_weight[output_bucket], net.l1_bias[output_bucket],
        sparse_ft_nibbles, sparse_nibbles_size, l1_activation);
    assert(std::all_of(l1_activation.begin(), l1_activation.end(), [](auto x) { return 0 <= x && x <= 1; }));

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