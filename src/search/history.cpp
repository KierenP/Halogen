#include "search/history.h"

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "search/data.h"

#include <array>

int16_t* PawnHistory::get(const BoardState& board, const SearchStackState*, Move move)
{
    const auto& stm = board.stm;
    const auto pawn_hash = board.pawn_key % pawn_states;
    const auto piece = enum_to<PieceType>(board.get_square_piece(move.from()));

    return &table[stm][pawn_hash][piece][move.to()];
}

int16_t* ThreatHistory::get(const BoardState& board, const SearchStackState*, Move move)
{
    const auto& stm = board.stm;
    const bool from_square_threat = (board.lesser_threats[KING] & SquareBB[move.from()]);
    const bool to_square_threat = (board.lesser_threats[KING] & SquareBB[move.to()]);

    return &table[stm][from_square_threat][to_square_threat][move.from()][move.to()];
}

int16_t* CaptureHistory::get(const BoardState& board, const SearchStackState*, Move move)
{
    const auto& stm = board.stm;
    const auto curr_piece = enum_to<PieceType>(board.get_square_piece(move.from()));
    const auto cap_piece = move.flag() == EN_PASSANT ? PAWN
        : move.is_promotion() && !move.is_capture()  ? PAWN // TODO: this preserves old behaviour
                                                     : enum_to<PieceType>(board.get_square_piece(move.to()));

    return &table[stm][curr_piece][move.to()][cap_piece];
}

int16_t* PieceMoveHistory::get(const BoardState& board, const SearchStackState*, Move move)
{
    const auto& stm = board.stm;
    const auto piece = enum_to<PieceType>(board.get_square_piece(move.from()));

    return &table[stm][piece][move.to()];
}

int16_t* PawnCorrHistory::get(const BoardState& board)
{
    const auto& stm = board.stm;
    const auto pawn_hash = board.pawn_key;
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

const int16_t* PawnCorrHistory::get(const BoardState& board) const
{
    const auto& stm = board.stm;
    const auto pawn_hash = board.pawn_key;
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

void PawnCorrHistory::add(const BoardState& board, int depth, int eval_diff)
{
    auto* entry = get(board);

    int change = std::clamp(
        eval_diff * depth * scale / 64, -correction_max * eval_scale() / 4, correction_max * eval_scale() / 4);
    *entry += change - *entry * abs(change) / (correction_max * eval_scale());
}

Score PawnCorrHistory::get_correction_score(const BoardState& board) const
{
    return *get(board) / eval_scale();
}

int16_t* NonPawnCorrHistory::get(const BoardState& board, Side side)
{
    const auto& stm = board.stm;
    const auto hash = board.non_pawn_key[side];
    return &table[stm][hash % hash_table_size];
}

void NonPawnCorrHistory::add(const BoardState& board, Side side, int depth, int eval_diff)
{
    auto* entry = get(board, side);

    int change = std::clamp(
        eval_diff * depth * scale / 64, -correction_max * eval_scale() / 4, correction_max * eval_scale() / 4);
    *entry += change - *entry * abs(change) / (correction_max * eval_scale());
}

Score NonPawnCorrHistory::get_correction_score(const BoardState& board, Side side)
{
    return *get(board, side) / eval_scale();
}

int16_t* PieceMoveCorrHistory::get(const BoardState& board, const SearchStackState* ss)
{
    const auto& stm = board.stm;
    // TODO: the following conditions preserve the old behaviour for handling null moves.
    const auto moved_piece = (ss - 1)->move != Move::Uninitialized ? enum_to<PieceType>((ss - 1)->moved_piece) : PAWN;
    const auto move_to = (ss - 1)->move != Move::Uninitialized ? (ss - 1)->move.to() : SQ_A1;
    return &table[!stm][moved_piece][move_to];
}

void PieceMoveCorrHistory::add(const BoardState& board, const SearchStackState* ss, int depth, int eval_diff)
{
    auto* entry = get(board, ss);

    int change = std::clamp(
        eval_diff * depth * scale / 64, -correction_max * eval_scale() / 4, correction_max * eval_scale() / 4);
    *entry += change - *entry * abs(change) / (correction_max * eval_scale());
}

Score PieceMoveCorrHistory::get_correction_score(const BoardState& board, const SearchStackState* ss)
{
    return *get(board, ss) / eval_scale();
}