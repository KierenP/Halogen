#pragma once

#include "bitboard/enum.h"
#include "network/accumulator.hpp"
#include "network/arch.hpp"

#include <array>
#include <cstdint>

// 12*64 unbucketed piece-square inputs, relative from each side's perspective
struct PieceSquareAccumulator
{
    struct Input
    {
        Square sq;
        Piece piece;
    };

    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const PieceSquareAccumulator& rhs) const
    {
        return side == rhs.side;
    }

    void add_input(const Input& input, Side view, const NN::network& net)
    {
        NN::add1(this->side[view], net.ft_weight[index(input.sq, input.piece, view)]);
    }

    void sub_input(const Input& input, Side view, const NN::network& net)
    {
        NN::sub1(this->side[view], net.ft_weight[index(input.sq, input.piece, view)]);
    }

    void add1sub1(
        const PieceSquareAccumulator& prev, const Input& a1, const Input& s1, Side view, const NN::network& net)
    {
        NN::add1sub1(side[view], prev.side[view], net.ft_weight[index(a1.sq, a1.piece, view)],
            net.ft_weight[index(s1.sq, s1.piece, view)]);
    }

    void add1sub2(const PieceSquareAccumulator& prev, const Input& a1, const Input& s1, const Input& s2, Side view,
        const NN::network& net)
    {
        NN::add1sub2(side[view], prev.side[view], net.ft_weight[index(a1.sq, a1.piece, view)],
            net.ft_weight[index(s1.sq, s1.piece, view)], net.ft_weight[index(s2.sq, s2.piece, view)]);
    }

    void add2sub2(const PieceSquareAccumulator& prev, const Input& a1, const Input& a2, const Input& s1,
        const Input& s2, Side view, const NN::network& net)
    {
        NN::add2sub2(side[view], prev.side[view], net.ft_weight[index(a1.sq, a1.piece, view)],
            net.ft_weight[index(a2.sq, a2.piece, view)], net.ft_weight[index(s1.sq, s1.piece, view)],
            net.ft_weight[index(s2.sq, s2.piece, view)]);
    }

    static size_t index(Square sq, Piece piece, Side view)
    {
        sq = view == WHITE ? sq : flip_square_vertical(sq);

        Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
        PieceType pieceType = enum_to<PieceType>(piece);

        return PSQT_OFFSET + relativeColor * 64 * 6 + pieceType * 64 + sq;
    }
};