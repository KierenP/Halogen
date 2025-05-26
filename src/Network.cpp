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
    [[maybe_unused]] constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 259, 432, 359, 431, 328, 30, 238, 43,
        522, 486, 182, 199, 142, 401, 4, 251, 545, 603, 552, 409, 276, 197, 168, 37, 60, 132, 68, 541, 263, 621, 244,
        241, 410, 363, 173, 450, 227, 201, 338, 32, 219, 538, 390, 514, 542, 46, 561, 50, 146, 286, 380, 558, 440, 489,
        455, 485, 177, 59, 296, 161, 280, 24, 305, 337, 127, 185, 281, 578, 193, 592, 403, 272, 383, 466, 370, 544, 448,
        285, 74, 5, 318, 151, 176, 20, 208, 508, 483, 248, 130, 239, 498, 291, 413, 39, 425, 15, 423, 549, 58, 152, 235,
        324, 220, 196, 368, 302, 392, 317, 275, 212, 585, 420, 620, 500, 116, 36, 134, 470, 107, 91, 599, 232, 428, 627,
        340, 506, 98, 422, 262, 348, 397, 608, 13, 246, 604, 493, 457, 437, 210, 572, 439, 1, 519, 298, 31, 288, 95,
        315, 258, 143, 356, 144, 245, 243, 384, 215, 141, 535, 112, 471, 117, 557, 18, 110, 624, 600, 153, 611, 320,
        229, 326, 309, 329, 164, 299, 563, 360, 452, 63, 282, 292, 71, 44, 463, 249, 480, 555, 349, 393, 361, 408, 49,
        469, 407, 279, 162, 526, 497, 381, 635, 590, 92, 137, 331, 100, 529, 476, 375, 556, 503, 88, 637, 160, 451, 178,
        123, 265, 632, 83, 247, 333, 430, 530, 73, 64, 581, 515, 330, 494, 367, 48, 319, 224, 565, 421, 207, 536, 321,
        183, 484, 344, 570, 195, 606, 310, 90, 28, 364, 617, 154, 634, 586, 571, 51, 346, 454, 618, 234, 576, 394, 158,
        559, 518, 77, 214, 357, 472, 587, 616, 374, 57, 297, 155, 303, 322, 255, 254, 66, 266, 194, 26, 175, 19, 334,
        54, 399, 443, 138, 85, 366, 223, 625, 524, 347, 531, 579, 306, 114, 342, 75, 402, 406, 631, 70, 623, 136, 441,
        84, 301, 355, 17, 619, 204, 551, 525, 589, 534, 200, 395, 264, 8, 109, 139, 567, 323, 38, 566, 78, 575, 389,
        386, 101, 260, 580, 47, 636, 119, 521, 496, 209, 205, 601, 198, 79, 312, 354, 236, 479, 55, 582, 614, 62, 495,
        97, 147, 378, 478, 528, 412, 190, 267, 6, 186, 352, 157, 447, 268, 225, 174, 12, 573, 553, 156, 583, 512, 140,
        82, 149, 203, 335, 102, 159, 449, 124, 76, 222, 591, 135, 594, 377, 336, 435, 179, 7, 491, 505, 462, 633, 42,
        487, 171, 27, 131, 3, 108, 242, 477, 574, 145, 584, 461, 165, 371, 307, 93, 473, 468, 516, 517, 122, 577, 167,
        560, 105, 111, 414, 597, 21, 539, 253, 527, 148, 424, 128, 226, 405, 588, 314, 16, 181, 612, 216, 22, 121, 184,
        41, 630, 416, 475, 595, 543, 564, 99, 213, 33, 89, 72, 311, 434, 332, 353, 404, 53, 256, 304, 202, 504, 113,
        510, 40, 387, 492, 464, 115, 206, 444, 610, 308, 438, 274, 120, 537, 554, 502, 626, 411, 520, 605, 10, 507, 96,
        56, 289, 87, 501, 221, 290, 35, 391, 546, 369, 488, 456, 547, 169, 0, 638, 596, 313, 228, 166, 188, 615, 25,
        639, 67, 269, 252, 607, 481, 69, 191, 358, 339, 150, 250, 341, 129, 294, 388, 11, 458, 482, 350, 81, 540, 103,
        230, 118, 613, 550, 609, 126, 14, 94, 23, 278, 351, 104, 261, 125, 287, 365, 511, 192, 442, 400, 273, 436, 446,
        172, 362, 433, 231, 427, 211, 622, 133, 417, 568, 379, 523, 283, 284, 532, 170, 415, 217, 61, 52, 343, 45, 628,
        65, 240, 598, 189, 257, 277, 376, 300, 80, 429, 237, 106, 398, 465, 533, 426, 2, 34, 316, 593, 233, 327, 293,
        385, 163, 509, 418, 382, 325, 629, 459, 271, 187, 460, 345, 86, 9, 445, 474, 490, 513, 602, 373, 562, 453, 467,
        372, 29, 548, 218, 396, 295, 419, 499, 180, 569, 270,
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

Score Network::SlowEval(const BoardState& board)
{
    Accumulator acc;
    acc.Recalculate(board);
    return Eval(board, acc);
}
