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
    [[maybe_unused]] constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 167, 7, 47, 185, 133, 70, 282, 176,
        283, 87, 360, 156, 228, 159, 193, 280, 326, 165, 371, 95, 141, 77, 327, 36, 308, 304, 264, 71, 126, 319, 225,
        163, 269, 274, 363, 296, 65, 84, 52, 365, 158, 74, 307, 305, 323, 267, 78, 194, 376, 219, 64, 236, 224, 93, 321,
        320, 2, 116, 90, 373, 248, 44, 114, 118, 212, 28, 66, 162, 152, 32, 216, 170, 325, 62, 101, 192, 175, 94, 200,
        169, 322, 198, 207, 215, 85, 82, 190, 232, 18, 41, 115, 79, 294, 171, 380, 168, 196, 275, 179, 362, 191, 331,
        359, 352, 117, 298, 5, 99, 231, 119, 347, 285, 374, 247, 301, 56, 205, 177, 271, 108, 354, 249, 125, 195, 189,
        187, 291, 351, 375, 312, 303, 33, 246, 206, 239, 334, 3, 83, 288, 21, 17, 199, 132, 309, 35, 355, 72, 302, 20,
        240, 311, 208, 377, 92, 37, 299, 276, 313, 310, 259, 229, 293, 382, 343, 345, 63, 16, 50, 368, 223, 46, 253,
        217, 135, 157, 287, 263, 151, 245, 372, 332, 6, 286, 237, 336, 188, 144, 104, 300, 59, 356, 324, 145, 202, 81,
        4, 370, 76, 25, 180, 51, 379, 121, 89, 260, 102, 279, 222, 73, 139, 69, 15, 105, 27, 34, 341, 348, 201, 261,
        112, 290, 23, 138, 124, 333, 340, 342, 155, 183, 120, 295, 100, 127, 68, 147, 49, 111, 98, 278, 130, 252, 113,
        14, 12, 361, 164, 226, 262, 26, 213, 10, 297, 140, 61, 330, 107, 314, 11, 209, 174, 166, 349, 58, 161, 211, 243,
        181, 328, 381, 106, 110, 137, 153, 265, 367, 234, 317, 129, 266, 337, 251, 154, 143, 178, 172, 277, 316, 24,
        103, 238, 19, 315, 284, 67, 86, 358, 142, 42, 22, 131, 203, 366, 43, 149, 268, 57, 160, 346, 80, 97, 136, 173,
        350, 40, 272, 242, 306, 335, 148, 250, 256, 270, 30, 357, 60, 134, 378, 220, 329, 182, 123, 122, 230, 1, 344,
        39, 383, 281, 75, 233, 273, 96, 109, 257, 338, 210, 88, 204, 364, 54, 197, 150, 186, 9, 244, 227, 29, 235, 318,
        146, 241, 31, 289, 339, 128, 353, 292, 53, 214, 45, 55, 91, 218, 48, 38, 258, 8, 221, 255, 184, 254, 369, 0, 13,
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
