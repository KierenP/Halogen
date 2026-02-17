#pragma once

#include "bitboard/enum.h"
#include "network/accumulator.hpp"
#include "network/arch.hpp"

#include <array>
#include <cstdint>

// 12*64*king-buckets inputs, relative from each side's perspective and horizontally mirrored
struct KingBucketPieceSquareAccumulator
{
    // represents a single input on one accumulator side
    struct Input
    {
        Square king_sq;
        Square piece_sq;
        Piece piece;
    };

    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const KingBucketPieceSquareAccumulator& rhs) const
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

    void add1sub1(const KingBucketPieceSquareAccumulator& prev, const Input& a1, const Input& s1, Side view,
        const NN::network& net)
    {
        NN::add1sub1(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)]);
    }

    void add1sub2(const KingBucketPieceSquareAccumulator& prev, const Input& a1, const Input& s1, const Input& s2,
        Side view, const NN::network& net)
    {
        NN::add1sub2(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)],
            net.ft_weight[index(s2.king_sq, s2.piece_sq, s2.piece, view)]);
    }

    void add2sub2(const KingBucketPieceSquareAccumulator& prev, const Input& a1, const Input& a2, const Input& s1,
        const Input& s2, Side view, const NN::network& net)
    {
        NN::add2sub2(side[view], prev.side[view], net.ft_weight[index(a1.king_sq, a1.piece_sq, a1.piece, view)],
            net.ft_weight[index(a2.king_sq, a2.piece_sq, a2.piece, view)],
            net.ft_weight[index(s1.king_sq, s1.piece_sq, s1.piece, view)],
            net.ft_weight[index(s2.king_sq, s2.piece_sq, s2.piece, view)]);
    }

    static int get_king_bucket(Square king_sq, Side view)
    {
        king_sq = view == WHITE ? king_sq : flip_square_vertical(king_sq);
        return KING_BUCKETS[king_sq];
    }

    static size_t index(Square king_sq, Square piece_sq, Piece piece, Side view)
    {
        piece_sq = view == WHITE ? piece_sq : flip_square_vertical(piece_sq);
        piece_sq = enum_to<File>(king_sq) <= FILE_D ? piece_sq : flip_square_horizontal(piece_sq);

        auto king_bucket = get_king_bucket(king_sq, view);
        Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
        PieceType pieceType = enum_to<PieceType>(piece);

        return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
    }
};