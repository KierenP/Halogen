#include "History.h"

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "GameState.h"
#include "Move.h"
#include "SearchData.h"

int16_t* PawnHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey() % pawn_states;
    const auto piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

    return &table[stm][pawn_hash][piece][move.GetTo()];
}

int16_t* ThreatHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& stm = position.Board().stm;
    const uint64_t& threat_mask = ss->threat_mask;
    const bool from_square_threat = threat_mask & SquareBB[move.GetFrom()];

    return &table[stm][from_square_threat][move.GetFrom()][move.GetTo()];
}

int16_t* CaptureHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto cap_piece = move.GetFlag() == EN_PASSANT ? PAWN : GetPieceType(position.Board().GetSquare(move.GetTo()));

    return &table[stm][curr_piece][move.GetTo()][cap_piece];
}

int16_t* PieceMoveHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

    return &table[stm][piece][move.GetTo()];
}

int16_t* PawnCorrHistory::get(const GameState& position)
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey();
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

const int16_t* PawnCorrHistory::get(const GameState& position) const
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey();
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

void PawnCorrHistory::add(const GameState& position, int depth, int eval_diff)
{
    auto* entry = get(position);

    int change = std::clamp(eval_diff * depth / 8, -correction_max * eval_scale / 4, correction_max * eval_scale / 4);
    *entry += change - *entry * abs(change) / (correction_max * eval_scale);
}

Score PawnCorrHistory::get_correction_score(const GameState& position) const
{
    return *get(position) / eval_scale;
}

int16_t* NonPawnCorrHistory::get(const GameState& position, Players side)
{
    const auto& stm = position.Board().stm;
    const auto hash = position.Board().non_pawn_key[side];
    return &table[stm][hash % hash_table_size];
}

void NonPawnCorrHistory::add(const GameState& position, Players side, int depth, int eval_diff)
{
    auto* entry = get(position, side);

    int change = std::clamp(eval_diff * depth / 8, -correction_max * eval_scale / 4, correction_max * eval_scale / 4);
    *entry += change - *entry * abs(change) / (correction_max * eval_scale);
}

Score NonPawnCorrHistory::get_correction_score(const GameState& position, Players side)
{
    return *get(position, side) / eval_scale;
}