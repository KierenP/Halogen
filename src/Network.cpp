#include "Network.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "incbin/incbin.h"
#include "nn/Accumulator.hpp"
#include "nn/Arch.hpp"
#include "nn/Features.hpp"
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
    alignas(64) std::array<std::array<std::array<float, L1_SIZE * 2>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
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
    [[maybe_unused]] constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 211, 489, 325, 208, 151, 44, 94, 452,
        143, 491, 98, 149, 473, 468, 222, 409, 113, 285, 139, 38, 499, 174, 351, 144, 24, 298, 6, 400, 283, 403, 58,
        299, 434, 427, 243, 185, 308, 170, 83, 304, 252, 482, 41, 241, 424, 183, 376, 309, 488, 379, 269, 265, 390, 125,
        88, 368, 108, 9, 234, 194, 458, 244, 227, 61, 261, 19, 431, 173, 450, 27, 59, 167, 122, 478, 52, 505, 162, 69,
        364, 348, 303, 350, 420, 75, 20, 177, 399, 343, 73, 35, 232, 388, 215, 421, 429, 95, 312, 294, 510, 79, 280,
        276, 50, 82, 92, 324, 106, 451, 331, 206, 317, 186, 230, 184, 263, 55, 402, 357, 469, 223, 500, 154, 80, 247,
        476, 501, 190, 441, 220, 355, 236, 330, 253, 287, 134, 120, 417, 28, 128, 425, 460, 179, 382, 142, 76, 188, 459,
        153, 332, 54, 147, 101, 422, 148, 238, 411, 26, 78, 235, 346, 338, 315, 445, 156, 8, 157, 91, 385, 161, 7, 168,
        171, 389, 172, 84, 199, 360, 266, 178, 461, 140, 180, 102, 96, 395, 406, 37, 467, 447, 428, 470, 217, 480, 479,
        256, 239, 353, 68, 166, 401, 272, 328, 454, 150, 104, 497, 136, 386, 169, 347, 126, 56, 475, 36, 440, 107, 51,
        214, 367, 123, 40, 46, 202, 165, 105, 336, 398, 224, 320, 229, 198, 67, 407, 196, 64, 225, 115, 430, 490, 152,
        487, 176, 159, 209, 282, 121, 408, 16, 251, 375, 249, 45, 77, 181, 245, 439, 437, 87, 12, 158, 228, 90, 25, 363,
        216, 433, 99, 267, 254, 43, 296, 30, 155, 191, 273, 404, 446, 11, 349, 163, 278, 371, 240, 507, 10, 511, 369,
        127, 313, 17, 111, 291, 423, 486, 316, 295, 286, 146, 129, 391, 300, 277, 448, 81, 85, 305, 259, 141, 226, 356,
        49, 33, 197, 492, 15, 72, 506, 53, 377, 42, 414, 205, 322, 34, 114, 334, 248, 327, 496, 32, 288, 416, 93, 274,
        118, 412, 344, 365, 318, 329, 271, 3, 145, 132, 372, 204, 97, 219, 86, 335, 14, 495, 109, 66, 419, 47, 201, 71,
        31, 464, 1, 323, 362, 192, 435, 275, 483, 342, 373, 361, 193, 307, 203, 89, 112, 314, 359, 405, 237, 262, 302,
        484, 380, 292, 218, 341, 432, 387, 74, 306, 354, 117, 345, 494, 394, 378, 231, 289, 392, 396, 337, 310, 393,
        195, 352, 116, 207, 133, 340, 135, 410, 339, 60, 413, 63, 18, 290, 264, 456, 508, 443, 418, 39, 281, 187, 293,
        426, 438, 65, 5, 279, 370, 384, 210, 2, 381, 436, 462, 242, 0, 124, 257, 326, 509, 466, 442, 465, 62, 221, 449,
        485, 200, 270, 453, 137, 498, 21, 444, 130, 358, 22, 301, 415, 57, 260, 160, 233, 48, 311, 455, 110, 471, 4, 23,
        268, 103, 297, 189, 374, 213, 333, 383, 258, 463, 321, 164, 481, 13, 397, 119, 182, 457, 504, 212, 502, 29, 175,
        474, 255, 472, 477, 246, 366, 503, 131, 70, 250, 284, 100, 319, 493, 138,
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
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE * 2; j++)
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
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS> l2_weight = {};
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

    NN::Accumulator::add1sub1(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[sub1_index]);
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

    NN::Accumulator::add1sub2(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[sub1_index],
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
    const Input& sub2, Players view)
{
    size_t add1_index = index(add1.king_sq, add1.piece_sq, add1.piece, view);
    size_t add2_index = index(add2.king_sq, add2.piece_sq, add2.piece, view);
    size_t sub1_index = index(sub1.king_sq, sub1.piece_sq, sub1.piece, view);
    size_t sub2_index = index(sub2.king_sq, sub2.piece_sq, sub2.piece, view);

    NN::Accumulator::add2sub2(next.side[view], prev.side[view], net.ft_weight[add1_index], net.ft_weight[add2_index],
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

void Accumulator::AddInput(const InputPair& input)
{
    AddInput({ input.w_king, input.piece_sq, input.piece }, WHITE);
    AddInput({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::AddInput(const Input& input, Players view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);
    NN::Accumulator::add1(side[view], net.ft_weight[side_index]);
}

void Accumulator::SubInput(const InputPair& input)
{
    SubInput({ input.w_king, input.piece_sq, input.piece }, WHITE);
    SubInput({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::SubInput(const Input& input, Players view)
{
    size_t side_index = index(input.king_sq, input.piece_sq, input.piece, view);
    NN::Accumulator::sub1(side[view], net.ft_weight[side_index]);
}

void Accumulator::Recalculate(const BoardState& board_)
{
    Recalculate(board_, WHITE);
    Recalculate(board_, BLACK);
    acc_is_valid = true;
}

void Accumulator::Recalculate(const BoardState& board_, Players view)
{
    side[view] = net.ft_bias;
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
        entry.acc.side = { net.ft_bias, net.ft_bias };
        entry.white_bb = {};
        entry.black_bb = {};
    }
}

bool Network::Verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = {};
    expected.side = { net.ft_bias, net.ft_bias };
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

void Network::StoreLazyUpdates(
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
            if (stm == WHITE)
            {
                acc.white_requires_recalculation = true;
            }
            else
            {
                acc.black_requires_recalculation = true;
            }

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

                acc.adds[0] = { w_king, b_king, king_end, from_piece };
                acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

                acc.subs[0] = { w_king, b_king, king_start, from_piece };
                acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
            }
            else if (move.IsCapture())
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

            acc.adds[0] = { w_king, b_king, king_end, from_piece };
            acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

            acc.subs[0] = { w_king, b_king, king_start, from_piece };
            acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
        }
        else if (move.IsCapture())
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
        if (move.IsPromotion() && move.IsCapture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else if (move.IsPromotion())
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
        else if (move.IsCapture() && move.GetFlag() == EN_PASSANT)
        {
            auto ep_capture_sq = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));

            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, ep_capture_sq, Piece(PAWN, !stm) };
        }
        else if (move.IsCapture())
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

void Network::ApplyLazyUpdates(const Accumulator& prev_acc, Accumulator& next_acc)
{
    if (next_acc.acc_is_valid)
    {
        return;
    }

    assert(!(next_acc.white_requires_recalculation && next_acc.black_requires_recalculation));

    if (next_acc.white_requires_recalculation)
    {
        table.Recalculate(next_acc, next_acc.board, WHITE, next_acc.board.GetKing(WHITE));

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
        table.Recalculate(next_acc, next_acc.board, BLACK, next_acc.board.GetKing(BLACK));

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

Score Network::Eval(const BoardState& board, const Accumulator& acc)
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(GetBitCount(board.GetAllPieces()));

    alignas(64) std::array<uint8_t, FT_SIZE> ft_activation;
    alignas(64) std::array<int16_t, FT_SIZE / 4> sparse_ft_nibbles;
    size_t sparse_nibbles_size = 0;
    NN::Features::FT_activation(acc.side[stm], acc.side[!stm], ft_activation, sparse_ft_nibbles, sparse_nibbles_size);
    assert(std::all_of(ft_activation.begin(), ft_activation.end(), [](auto x) { return x <= 127; }));

    alignas(64) std::array<float, L1_SIZE * 2> l1_activation;
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

Score Network::SlowEval(const BoardState& board)
{
    Accumulator acc;
    acc.Recalculate(board);
    return Eval(board, acc);
}
