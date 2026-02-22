#pragma once

#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "network/arch.hpp"
#include "network/inputs/king_bucket.h"
#include "network/simd/accumulator.hpp"

#include <array>
#include <cstdint>

namespace NN::KingBucket
{

class AccumulatorTable;

// Accumulator for king-bucketed piece-square inputs.
// The 'side' array stores: bias + king-bucketed weights combined.
struct KingBucketAccumulator
{
    // represents a pair of inputs (one for each side)
    struct InputPair
    {
        Square w_king;
        Square b_king;
        Square piece_sq;
        Piece piece;
    };

    // represents a single input on one accumulator side
    struct Input
    {
        Square king_sq;
        Square piece_sq;
        Piece piece;
    };

    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const KingBucketAccumulator& rhs) const
    {
        return side == rhs.side;
    }

    void add_input(const Input& input, Side view, const NN::network& net)
    {
        NN::add1(this->side[view], net.ft_weight[index(input.king_sq, input.piece_sq, input.piece, view)]);
    }

    void sub_input(const Input& input, Side view, const NN::network& net)
    {
        NN::sub1(this->side[view], net.ft_weight[index(input.king_sq, input.piece_sq, input.piece, view)]);
    }

    void add1sub1(
        const KingBucketAccumulator& prev, const Input& a1, const Input& s1, Side view, const NN::network& net)
    {
        NN::add1sub1(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)]);
    }

    void add1sub1(
        const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, Side view, const NN::network& net)
    {
        auto king = view == WHITE ? a1.w_king : a1.b_king;
        add1sub1(prev, { king, a1.piece_sq, a1.piece }, { king, s1.piece_sq, s1.piece }, view, net);
    }

    void add1sub1(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const NN::network& net)
    {
        add1sub1(prev, a1, s1, WHITE, net);
        add1sub1(prev, a1, s1, BLACK, net);
    }

    void add1sub2(const KingBucketAccumulator& prev, const Input& a1, const Input& s1, const Input& s2, Side view,
        const NN::network& net)
    {
        NN::add1sub2(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)],
            net.ft_weight[index(s2.king_sq, s2.piece_sq, s2.piece, view)]);
    }

    void add1sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const InputPair& s2,
        const NN::network& net)
    {
        add1sub2(prev, a1, s1, s2, WHITE, net);
        add1sub2(prev, a1, s1, s2, BLACK, net);
    }

    void add1sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& s1, const InputPair& s2,
        Side view, const NN::network& net)
    {
        auto king = view == WHITE ? a1.w_king : a1.b_king;
        add1sub2(prev, { king, a1.piece_sq, a1.piece }, { king, s1.piece_sq, s1.piece },
            { king, s2.piece_sq, s2.piece }, view, net);
    }

    void add2sub2(const KingBucketAccumulator& prev, const Input& a1, const Input& a2, const Input& s1, const Input& s2,
        Side view, const NN::network& net)
    {
        NN::add2sub2(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(a2.king_sq, a2.piece_sq, a2.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)],
            net.ft_weight[index(s2.king_sq, s2.piece_sq, s2.piece, view)]);
    }

    void add2sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& a2, const InputPair& s1,
        const InputPair& s2, Side view, const NN::network& net)
    {
        auto king = view == WHITE ? a1.w_king : a1.b_king;
        add2sub2(prev, { king, a1.piece_sq, a1.piece }, { king, a2.piece_sq, a2.piece },
            { king, s1.piece_sq, s1.piece }, { king, s2.piece_sq, s2.piece }, view, net);
    }

    void add2sub2(const KingBucketAccumulator& prev, const InputPair& a1, const InputPair& a2, const InputPair& s1,
        const InputPair& s2, const NN::network& net)
    {
        add2sub2(prev, a1, a2, s1, s2, WHITE, net);
        add2sub2(prev, a1, a2, s1, s2, BLACK, net);
    }

    void recalculate_from_scratch(const BoardState& board_, const NN::network& net)
    {
        // use the board we pass in as an argument rather than the one stored in the accumulator, since the stored one
        // may be stale

        *this = {};
        side[WHITE] = net.ft_bias;
        side[BLACK] = net.ft_bias;

        auto w_king_sq = board_.get_king_sq(WHITE);
        auto b_king_sq = board_.get_king_sq(BLACK);

        for (int i = 0; i < N_PIECES; i++)
        {
            Piece piece = static_cast<Piece>(i);
            uint64_t bb = board_.get_pieces_bb(piece);

            while (bb)
            {
                Square sq = lsbpop(bb);
                add_input({ w_king_sq, sq, piece }, WHITE, net);
                add_input({ b_king_sq, sq, piece }, BLACK, net);
            }
        }

        acc_is_valid = true;
    }

    void store_lazy_updates(const BoardState& prev_move_board, const BoardState& post_move_board, Move move)
    {
        assert(move != Move::Uninitialized);

        acc_is_valid = false;
        white_requires_recalculation = false;
        black_requires_recalculation = false;
        adds = {};
        n_adds = 0;
        subs = {};
        n_subs = 0;
        board = {};

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
            if (KingBucket::get_king_bucket(from_sq, stm) != KingBucket::get_king_bucket(to_sq, stm)
                || ((enum_to<File>(from_sq) <= FILE_D) ^ (enum_to<File>(to_sq) <= FILE_D)))
            {
                if (stm == WHITE)
                {
                    white_requires_recalculation = true;
                }
                else
                {
                    black_requires_recalculation = true;
                }

                board = post_move_board;
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

            n_adds = 2;
            n_subs = 2;

            adds[0] = { w_king, b_king, king_end, from_piece };
            adds[1] = { w_king, b_king, rook_end, cap_piece };

            subs[0] = { w_king, b_king, king_start, from_piece };
            subs[1] = { w_king, b_king, rook_start, cap_piece };
        }
        else if (move.is_promotion() && move.is_capture())
        {
            n_adds = 1;
            n_subs = 2;

            adds[0] = { w_king, b_king, to_sq, to_piece };

            subs[0] = { w_king, b_king, from_sq, from_piece };
            subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else if (move.is_promotion())
        {
            n_adds = 1;
            n_subs = 1;

            adds[0] = { w_king, b_king, to_sq, to_piece };

            subs[0] = { w_king, b_king, from_sq, from_piece };
        }
        else if (move.is_capture() && move.flag() == EN_PASSANT)
        {
            auto ep_capture_sq = get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()));

            n_adds = 1;
            n_subs = 2;

            adds[0] = { w_king, b_king, to_sq, from_piece };

            subs[0] = { w_king, b_king, from_sq, from_piece };
            subs[1] = { w_king, b_king, ep_capture_sq, get_piece(PAWN, !stm) };
        }
        else if (move.is_capture())
        {
            n_adds = 1;
            n_subs = 2;

            adds[0] = { w_king, b_king, to_sq, from_piece };

            subs[0] = { w_king, b_king, from_sq, from_piece };
            subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else
        {
            n_adds = 1;
            n_subs = 1;

            adds[0] = { w_king, b_king, to_sq, from_piece };

            subs[0] = { w_king, b_king, from_sq, from_piece };
        }
    }

    void apply_lazy_updates(const KingBucketAccumulator& prev_acc, AccumulatorTable& table, const NN::network& net);

    bool acc_is_valid = false;
    bool white_requires_recalculation = false;
    bool black_requires_recalculation = false;
    std::array<InputPair, 2> adds = {};
    size_t n_adds = 0;
    std::array<InputPair, 2> subs = {};
    size_t n_subs = 0;
    BoardState board;
};

// An accumulator table entry caches the king-bucketed accumulator for a particular king position.
// When copying to the live accumulator, piece-square inputs are added on top from scratch.
struct AccumulatorTableEntry
{
    KingBucketAccumulator acc;
    std::array<uint64_t, N_PIECES> white_bb = {};
    std::array<uint64_t, N_PIECES> black_bb = {};
};

// A cache of king-bucketed accumulators for each king bucket. When we want to recalculate the
// king-bucketed accumulator, we use this as a base rather than initializing from scratch.
struct AccumulatorTable
{
    std::array<AccumulatorTableEntry, KING_BUCKET_COUNT * 2> king_bucket = {};

    void recalculate(
        KingBucketAccumulator& acc, const BoardState& board, Side side, Square king_sq, const NN::network& net)
    {
        // we need to keep a separate entry for when the king is on each side of the board
        auto& entry = king_bucket[KingBucket::get_king_bucket(king_sq, side)
            + (enum_to<File>(king_sq) <= FILE_D ? 0 : KingBucket::KING_BUCKET_COUNT)];
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

        acc.side[side] = entry.acc.side[side];
    }

    void reset_table(const std::array<int16_t, NN::FT_SIZE>& ft_bias)
    {
        for (auto& entry : king_bucket)
        {
            entry.acc = {};
            entry.acc.side = { ft_bias, ft_bias };
            entry.white_bb = {};
            entry.black_bb = {};
        }
    }
};
}