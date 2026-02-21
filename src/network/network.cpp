#include "network.h"

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "network/accumulator.hpp"
#include "network/arch.hpp"
#include "network/inference.hpp"
#include "network/inputs/king_bucket.h"
#include "network/inputs/threat.h"
#include "network/threat_inputs.h"
#include "third-party/incbin/incbin.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>

#if defined(USE_SSE4)
// IWYU is confused and thinks we need this
#include <immintrin.h>
#endif

namespace NN
{

#undef INCBIN_ALIGNMENT
#define INCBIN_ALIGNMENT 64
INCBIN(Net, EVALFILE);

const network& net = reinterpret_cast<const network&>(*gNetData);

[[maybe_unused]] auto verify_network_size = []
{
    if (sizeof(network) != gNetSize)
    {
        std::cout << "Error: embedded network is not the expected size. Expected " << sizeof(network)
                  << " bytes actual " << gNetSize << " bytes." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return true;
}();

// --- Helper to apply fused updates to both sub-accumulators for one side ---

void add1sub1(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& s1, Side view)
{
    auto king = view == WHITE ? a1.w_king : a1.b_king;
    next.king_bucket.add1sub1(
        prev.king_bucket, { king, a1.piece_sq, a1.piece }, { king, s1.piece_sq, s1.piece }, view, net);
}

void add1sub2(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& s1, const InputPair& s2,
    Side view)
{
    auto king = view == WHITE ? a1.w_king : a1.b_king;
    next.king_bucket.add1sub2(prev.king_bucket, { king, a1.piece_sq, a1.piece }, { king, s1.piece_sq, s1.piece },
        { king, s2.piece_sq, s2.piece }, view, net);
}

void add2sub2(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& a2, const InputPair& s1,
    const InputPair& s2, Side view)
{
    auto king = view == WHITE ? a1.w_king : a1.b_king;
    next.king_bucket.add2sub2(prev.king_bucket, { king, a1.piece_sq, a1.piece }, { king, a2.piece_sq, a2.piece },
        { king, s1.piece_sq, s1.piece }, { king, s2.piece_sq, s2.piece }, view, net);
}

// Apply fused updates to both sub-accumulators for both sides
void add1sub1(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& s1)
{
    add1sub1(prev, next, a1, s1, WHITE);
    add1sub1(prev, next, a1, s1, BLACK);
}

void add1sub2(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& s1, const InputPair& s2)
{
    add1sub2(prev, next, a1, s1, s2, WHITE);
    add1sub2(prev, next, a1, s1, s2, BLACK);
}

void add2sub2(const Accumulator& prev, Accumulator& next, const InputPair& a1, const InputPair& a2, const InputPair& s1,
    const InputPair& s2)
{
    add2sub2(prev, next, a1, a2, s1, s2, WHITE);
    add2sub2(prev, next, a1, a2, s1, s2, BLACK);
}

void Accumulator::recalculate(const BoardState& board_)
{
    recalculate(board_, WHITE);
    recalculate(board_, BLACK);
    acc_is_valid = true;
}

void Accumulator::recalculate(const BoardState& board_, Side view)
{
    king_bucket.side[view] = net.ft_bias;
    auto king = board_.get_king_sq(view);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board_.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            king_bucket.add_input({ king, sq, piece }, view, net);
        }
    }
}

void AccumulatorTable::recalculate(Accumulator& acc, const BoardState& board, Side side, Square king_sq)
{
    // we need to keep a separate entry for when the king is on each side of the board
    auto& entry = king_bucket[KingBucketPieceSquareAccumulator::get_king_bucket(king_sq, side)
        + (enum_to<File>(king_sq) <= FILE_D ? 0 : KING_BUCKET_COUNT)];
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
            entry.acc.add_input({ king_sq, sq, piece }, side, net);
        }

        while (to_sub)
        {
            auto sq = lsbpop(to_sub);
            entry.acc.sub_input({ king_sq, sq, piece }, side, net);
        }

        old_bb = new_bb;
    }

    acc.king_bucket.side[side] = entry.acc.side[side];
}

void Network::reset_new_search(const BoardState& board, Accumulator& acc)
{
    acc.recalculate(board);
    acc.threats.compute(board, net);

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
    expected.king_bucket.side = { net.ft_bias, net.ft_bias };
    auto w_king = board.get_king_sq(WHITE);
    auto b_king = board.get_king_sq(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            expected.king_bucket.add_input({ w_king, sq, piece }, WHITE, net);
            expected.king_bucket.add_input({ b_king, sq, piece }, BLACK, net);
        }
    }

    expected.threats.compute(board, net);

    return expected == acc;
}

void Network::store_lazy_updates(
    const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move)
{
    assert(move != Move::Uninitialized);

    acc.acc_is_valid = false;
    acc.white_requires_recalculation = false;
    acc.black_requires_recalculation = false;
    acc.white_threats_requires_recalculation = false;
    acc.black_threats_requires_recalculation = false;
    acc.adds = {};
    acc.n_adds = 0;
    acc.subs = {};
    acc.n_subs = 0;
    acc.board = {};

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
        if ((enum_to<File>(from_sq) <= FILE_D) ^ (enum_to<File>(to_sq) <= FILE_D))
        {
            if (stm == WHITE)
            {
                acc.white_threats_requires_recalculation = true;
            }
            else
            {
                acc.black_threats_requires_recalculation = true;
            }
        }

        if (KingBucketPieceSquareAccumulator::get_king_bucket(from_sq, stm)
                != KingBucketPieceSquareAccumulator::get_king_bucket(to_sq, stm)
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
        }
    }

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
    else if (move.is_promotion() && move.is_capture())
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

    {
        uint64_t sub_bb = 0;
        for (size_t i = 0; i < acc.n_subs; i++)
            sub_bb |= SquareBB[acc.subs[i].piece_sq];
        uint64_t add_bb = 0;
        for (size_t i = 0; i < acc.n_adds; i++)
            add_bb |= SquareBB[acc.adds[i].piece_sq];

        acc.threats.compute_threat_deltas(prev_move_board, post_move_board, sub_bb, add_bb);
    }
}

void Network::apply_lazy_updates(const Accumulator& prev_acc, Accumulator& next_acc)
{
    if (next_acc.acc_is_valid)
    {
        return;
    }

    assert(!(next_acc.white_requires_recalculation && next_acc.black_requires_recalculation));
    assert(!(next_acc.white_threats_requires_recalculation && next_acc.black_threats_requires_recalculation));

    if (next_acc.white_requires_recalculation)
    {
        // King changed bucket for white — recalculate from table (sets kb + psq for WHITE)
        table.recalculate(next_acc, next_acc.board, WHITE, next_acc.board.get_king_sq(WHITE));

        // Incrementally update BLACK (kb + psq fused)
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
            add1sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], BLACK);
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
            add1sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1], BLACK);
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
            add2sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1], BLACK);
    }
    else if (next_acc.black_requires_recalculation)
    {
        // King changed bucket for black — recalculate from table (sets kb + psq for BLACK)
        table.recalculate(next_acc, next_acc.board, BLACK, next_acc.board.get_king_sq(BLACK));

        // Incrementally update WHITE (kb + psq fused)
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
            add1sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], WHITE);
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
            add1sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1], WHITE);
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
            add2sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1], WHITE);
    }
    else
    {
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            add1sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0]);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            add1sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            add2sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 0 && next_acc.n_subs == 0)
        {
            // null move
            next_acc.king_bucket.side = prev_acc.king_bucket.side;
        }
    }

    // Apply threat deltas (works for all cases including null move where there are 0 deltas)
    if (next_acc.white_threats_requires_recalculation)
    {
        next_acc.threats.compute(next_acc.board, net, WHITE);
        next_acc.threats.apply_deltas(prev_acc.threats, net, BLACK);
    }
    else if (next_acc.black_threats_requires_recalculation)
    {
        next_acc.threats.compute(next_acc.board, net, BLACK);
        next_acc.threats.apply_deltas(prev_acc.threats, net, WHITE);
    }
    else
    {
        next_acc.threats.apply_deltas(prev_acc.threats, net);
    }

    next_acc.acc_is_valid = true;
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

// Combine the king-bucketed and piece-square sub-accumulators with the bias, then add threat features
// to produce the final STM/NSTM accumulator values for inference.
void combine_accumulators(const BoardState& board, const Accumulator& acc, std::array<int16_t, FT_SIZE>& out_stm,
    std::array<int16_t, FT_SIZE>& out_nstm)
{
    auto stm = board.stm;

    // king_bucket.side stores bias + king-bucketed + piece-square combined
    out_stm = acc.king_bucket.side[stm];
    out_nstm = acc.king_bucket.side[!stm];

    // Add incrementally updated threat features
    NN::add1(out_stm, acc.threats.side[stm]);
    NN::add1(out_nstm, acc.threats.side[!stm]);
}

Score Network::eval(const BoardState& board, const Accumulator& acc)
{
    auto output_bucket = calculate_output_bucket(std::popcount(board.get_pieces_bb()));

    // Combine all sub-accumulators + threats into final STM/NSTM values
    alignas(64) std::array<int16_t, FT_SIZE> combined_stm;
    alignas(64) std::array<int16_t, FT_SIZE> combined_nstm;
    combine_accumulators(board, acc, combined_stm, combined_nstm);

    alignas(64) std::array<uint8_t, FT_SIZE> ft_activation;
    alignas(64) std::array<int16_t, FT_SIZE / 4> sparse_ft_nibbles;
    size_t sparse_nibbles_size = 0;
    NN::Features::FT_activation(combined_stm, combined_nstm, ft_activation, sparse_ft_nibbles, sparse_nibbles_size);
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

Score Network::slow_eval(const BoardState& board)
{
    Accumulator acc;
    acc.recalculate(board);
    acc.threats.compute(board, net);
    return eval(board, acc);
}

}