#include "piece_count.h"

#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "network/inputs/piece_count.h"
#include "network/simd/accumulator.hpp"

#include <bit>
#include <cassert>

namespace NN::PieceCount
{

bool PieceCountAccumulator::operator==(const PieceCountAccumulator& rhs) const
{
    return side == rhs.side;
}

// Helper: add all active features for a perspective
static void add_all_features(std::array<int16_t, FT_SIZE>& acc_side,
    const std::array<std::array<uint8_t, N_PIECE_TYPES>, N_SIDES>& piece_counts, Side perspective,
    const NN::network& net)
{
    emit_features_for_side(
        piece_counts[perspective], 0, [&](size_t idx) { NN::add1(acc_side, net.ft_piece_count_weight[idx]); });
    emit_features_for_side(piece_counts[!perspective], PIECE_COUNT_PER_SIDE,
        [&](size_t idx) { NN::add1(acc_side, net.ft_piece_count_weight[idx]); });
}

void PieceCountAccumulator::recalculate_from_scratch(const BoardState& board, const NN::network& net)
{
    *this = {};
    auto counts = compute_piece_counts(board);

    add_all_features(side[WHITE], counts, WHITE, net);
    add_all_features(side[BLACK], counts, BLACK, net);

    acc_is_valid = true;
}

// Lookup table: threshold_sub[piece_type][old_count] → local feature offset deactivated when
// count drops from old_count to old_count-1. 0xFF = no threshold crossed.
// Assumes legal positions (pawn count 1..8 always crosses).
static constexpr uint8_t NO_THRESHOLD = 0xFF;
// clang-format off
static constexpr uint8_t THRESHOLD_SUB[N_PIECE_TYPES][11] = {
    //          0     1     2     3     4     5     6     7     8     9    10
    /* PAWN   */ { NO_THRESHOLD, 14, 13, 12, 11, 10,  9,  8,  7, NO_THRESHOLD, NO_THRESHOLD },
    /* KNIGHT */ { NO_THRESHOLD,  6,  5, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* BISHOP */ { NO_THRESHOLD,  4,  3, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* ROOK   */ { NO_THRESHOLD,  2,  1, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* QUEEN  */ { NO_THRESHOLD,  0, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* KING   */ { NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
};

// Lookup table: threshold_add[piece_type][old_count] → local feature offset activated when
// count increases from old_count to old_count+1.
static constexpr uint8_t THRESHOLD_ADD[N_PIECE_TYPES][11] = {
    //          0     1     2     3     4     5     6     7     8     9    10
    /* PAWN   */ { 14, 13, 12, 11, 10,  9,  8,  7, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* KNIGHT */ {  6,  5, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* BISHOP */ {  4,  3, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* ROOK   */ {  2,  1, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* QUEEN  */ {  0, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
    /* KING   */ { NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD, NO_THRESHOLD },
};
// clang-format on

// Convert a side-relative local feature offset to a per-perspective feature index.
// "my" pieces are at base 0, "opponent's" are at base PIECE_COUNT_PER_SIDE.
static constexpr uint8_t perspective_feature(Side piece_owner, Side perspective, uint8_t local_offset)
{
    return local_offset + PIECE_COUNT_PER_SIDE * (piece_owner != perspective);
}

void PieceCountAccumulator::store_lazy_updates(
    const BoardState& prev_board, const BoardState& /*post_board*/, Move move)
{
    acc_is_valid = false;
    delta = PieceCountDelta::UNCHANGED;

    switch (move.flag())
    {
    case QUIET:
    case PAWN_DOUBLE_MOVE:
    case A_SIDE_CASTLE:
    case H_SIDE_CASTLE:
        // No piece counts change — accumulator stays identical to prev
        return;

    case CAPTURE:
    {
        auto cap_piece = prev_board.get_square_piece(move.to());
        auto cap_type = enum_to<PieceType>(cap_piece);
        auto cap_side = enum_to<Side>(cap_piece);
        auto old_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(cap_type, cap_side)));

        auto offset = THRESHOLD_SUB[cap_type][old_count];
        if (offset != NO_THRESHOLD)
        {
            sub_feature[WHITE][0] = perspective_feature(cap_side, WHITE, offset);
            sub_feature[BLACK][0] = perspective_feature(cap_side, BLACK, offset);
            delta = PieceCountDelta::SUB1;
        }
        else
        {
            delta = PieceCountDelta::COPY;
        }
        break;
    }

    case EN_PASSANT:
    {
        // EP always captures a pawn, and pawn count 1..8 always crosses a threshold
        auto cap_side = !prev_board.stm;
        auto old_pawn_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(PAWN, cap_side)));
        auto offset = THRESHOLD_SUB[PAWN][old_pawn_count];

        sub_feature[WHITE][0] = perspective_feature(cap_side, WHITE, offset);
        sub_feature[BLACK][0] = perspective_feature(cap_side, BLACK, offset);
        delta = PieceCountDelta::SUB1;
        break;
    }

    case KNIGHT_PROMOTION:
    case BISHOP_PROMOTION:
    case ROOK_PROMOTION:
    case QUEEN_PROMOTION:
    {
        // Pawn sub always crosses a threshold (pawn count 1..8)
        auto stm = prev_board.stm;
        auto promo_type = static_cast<PieceType>(move.flag() - KNIGHT_PROMOTION + KNIGHT);
        auto old_pawn_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(PAWN, stm)));
        auto pawn_sub = THRESHOLD_SUB[PAWN][old_pawn_count];

        sub_feature[WHITE][0] = perspective_feature(stm, WHITE, pawn_sub);
        sub_feature[BLACK][0] = perspective_feature(stm, BLACK, pawn_sub);

        // Promoted piece count increases (may not cross if already above max threshold)
        auto old_promo_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(promo_type, stm)));
        auto promo_add = THRESHOLD_ADD[promo_type][old_promo_count];

        if (promo_add != NO_THRESHOLD)
        {
            add_feature[WHITE] = perspective_feature(stm, WHITE, promo_add);
            add_feature[BLACK] = perspective_feature(stm, BLACK, promo_add);
            delta = PieceCountDelta::ADD1SUB1;
        }
        else
        {
            delta = PieceCountDelta::SUB1;
        }
        break;
    }

    case KNIGHT_PROMOTION_CAPTURE:
    case BISHOP_PROMOTION_CAPTURE:
    case ROOK_PROMOTION_CAPTURE:
    case QUEEN_PROMOTION_CAPTURE:
    {
        // Pawn sub always crosses a threshold (pawn count 1..8)
        auto stm = prev_board.stm;
        auto promo_type = static_cast<PieceType>(move.flag() - KNIGHT_PROMOTION_CAPTURE + KNIGHT);
        auto old_pawn_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(PAWN, stm)));
        auto pawn_sub = THRESHOLD_SUB[PAWN][old_pawn_count];

        sub_feature[WHITE][0] = perspective_feature(stm, WHITE, pawn_sub);
        sub_feature[BLACK][0] = perspective_feature(stm, BLACK, pawn_sub);

        // Captured piece count decreases
        auto cap_piece = prev_board.get_square_piece(move.to());
        auto cap_type = enum_to<PieceType>(cap_piece);
        auto cap_side = enum_to<Side>(cap_piece);
        auto old_cap_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(cap_type, cap_side)));
        auto cap_sub = THRESHOLD_SUB[cap_type][old_cap_count];

        // Promoted piece count increases
        auto old_promo_count = static_cast<uint8_t>(std::popcount(prev_board.get_pieces_bb(promo_type, stm)));
        auto promo_add = THRESHOLD_ADD[promo_type][old_promo_count];

        bool has_cap_sub = (cap_sub != NO_THRESHOLD);
        bool has_add = (promo_add != NO_THRESHOLD);

        if (has_cap_sub)
        {
            sub_feature[WHITE][1] = perspective_feature(cap_side, WHITE, cap_sub);
            sub_feature[BLACK][1] = perspective_feature(cap_side, BLACK, cap_sub);
        }
        if (has_add)
        {
            add_feature[WHITE] = perspective_feature(stm, WHITE, promo_add);
            add_feature[BLACK] = perspective_feature(stm, BLACK, promo_add);
        }

        // pawn sub always present (n_subs >= 1), so the delta is determined by cap_sub and promo_add
        static constexpr PieceCountDelta PROMO_CAP_TABLE[2][2] = {
            //              !has_add              has_add
            /* !has_cap */ { PieceCountDelta::SUB1, PieceCountDelta::ADD1SUB1 },
            /*  has_cap */ { PieceCountDelta::SUB2, PieceCountDelta::ADD1SUB2 },
        };
        delta = PROMO_CAP_TABLE[has_cap_sub][has_add];
        break;
    }

    default:
        assert(false);
        return;
    }
}

void PieceCountAccumulator::apply_lazy_updates(const PieceCountAccumulator& prev, const NN::network& net)
{
    switch (delta)
    {
    case PieceCountDelta::UNCHANGED:
    case PieceCountDelta::COPY:
        side = prev.side;
        break;

    case PieceCountDelta::SUB1:
        side = prev.side;
        NN::sub1(side[WHITE], net.ft_piece_count_weight[sub_feature[WHITE][0]]);
        NN::sub1(side[BLACK], net.ft_piece_count_weight[sub_feature[BLACK][0]]);
        break;

    case PieceCountDelta::SUB2:
        side = prev.side;
        NN::sub1(side[WHITE], net.ft_piece_count_weight[sub_feature[WHITE][0]]);
        NN::sub1(side[WHITE], net.ft_piece_count_weight[sub_feature[WHITE][1]]);
        NN::sub1(side[BLACK], net.ft_piece_count_weight[sub_feature[BLACK][0]]);
        NN::sub1(side[BLACK], net.ft_piece_count_weight[sub_feature[BLACK][1]]);
        break;

    case PieceCountDelta::ADD1SUB1:
        NN::add1sub1(side[WHITE], prev.side[WHITE], net.ft_piece_count_weight[add_feature[WHITE]],
            net.ft_piece_count_weight[sub_feature[WHITE][0]]);
        NN::add1sub1(side[BLACK], prev.side[BLACK], net.ft_piece_count_weight[add_feature[BLACK]],
            net.ft_piece_count_weight[sub_feature[BLACK][0]]);
        break;

    case PieceCountDelta::ADD1SUB2:
        NN::add1sub2(side[WHITE], prev.side[WHITE], net.ft_piece_count_weight[add_feature[WHITE]],
            net.ft_piece_count_weight[sub_feature[WHITE][0]], net.ft_piece_count_weight[sub_feature[WHITE][1]]);
        NN::add1sub2(side[BLACK], prev.side[BLACK], net.ft_piece_count_weight[add_feature[BLACK]],
            net.ft_piece_count_weight[sub_feature[BLACK][0]], net.ft_piece_count_weight[sub_feature[BLACK][1]]);
        break;
    }

    acc_is_valid = true;
}

} // namespace NN::PieceCount
