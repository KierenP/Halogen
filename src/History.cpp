#include "History.h"
#include "BitBoardDefine.h"
#include "Move.h"
#include "SearchData.h"

int16_t* ButterflyHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    return &table[stm][move.GetFrom()][move.GetTo()];
}

int16_t* CountermoveHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 1)->move;

    if (counter == Move::Uninitialized)
    {
        return nullptr;
    }

    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto counter_piece = GetPieceType((ss - 1)->moved_piece);

    return &table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

int16_t* CaptureHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto cap_piece = move.GetFlag() == EN_PASSANT ? PAWN : GetPieceType(position.Board().GetSquare(move.GetTo()));

    return &table[stm][curr_piece][move.GetTo()][cap_piece];
}

int16_t* CorrectionHistory::get(const GameState& position)
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey();
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

void CorrectionHistory::add(const GameState& position, int depth, int eval_diff)
{
    auto* entry = get(position);
    // give a higher weight to high depth observations
    auto ewa_weight = std::clamp(depth, 1, 16);
    // calculate the EWA, clamping to the min/max accordingly
    auto new_value
        = (*entry * (ewa_weight_scale - ewa_weight) + eval_diff * eval_scale * ewa_weight) / ewa_weight_scale;
    *entry = std::clamp<int>(new_value, -correction_max.value() * eval_scale, correction_max.value() * eval_scale);
}

void CorrectionHistory::reset()
{
    memset(table, 0, sizeof(table));
}

Score CorrectionHistory::get_correction_score(const GameState& position) const
{
    return *const_cast<CorrectionHistory*>(this)->get(position) / eval_scale;
}
