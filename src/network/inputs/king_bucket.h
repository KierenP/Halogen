#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"

namespace NN::KingBucket
{

// clang-format off
constexpr std::array<size_t, N_SQUARES> KING_BUCKETS = {
    0, 1, 2, 3, 3, 2, 1, 0,
    4, 4, 5, 5, 5, 5, 4, 4,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
};
// clang-format on

constexpr size_t KING_BUCKET_COUNT = []()
{
    auto [min, max] = std::minmax_element(KING_BUCKETS.begin(), KING_BUCKETS.end());
    return *max - *min + 1;
}();

constexpr int get_king_bucket(Square king_sq, Side view)
{
    king_sq = view == WHITE ? king_sq : flip_square_vertical(king_sq);
    return KING_BUCKETS[king_sq];
}

constexpr size_t index(Square king_sq, Square piece_sq, Piece piece, Side view)
{
    piece_sq = view == WHITE ? piece_sq : flip_square_vertical(piece_sq);
    piece_sq = enum_to<File>(king_sq) <= FILE_D ? piece_sq : flip_square_horizontal(piece_sq);

    auto king_bucket = get_king_bucket(king_sq, view);
    Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
    PieceType pieceType = enum_to<PieceType>(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

inline constexpr size_t TOTAL_KING_BUCKET_INPUTS = KING_BUCKET_COUNT * 64 * 6 * 2;

}