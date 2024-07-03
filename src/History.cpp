#include "History.h"
#include "BitBoardDefine.h"
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
    const auto counter_piece = GetPieceType(position.Board().GetSquare(counter.GetTo()));

    return &table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

void History::reset()
{
    std::apply([](auto&... table) { (table.reset(), ...); }, tables_);
}

int History::get(const GameState& position, const SearchStackState* ss, Move move)
{
    auto get_value = [&](auto& table)
    {
        auto* value = table.get(position, ss, move);
        return value ? *value : 0;
    };

    return std::apply([&](auto&... table) { return (get_value(table) + ...); }, tables_)
        / std::tuple_size_v<decltype(tables_)>;
}

void History::add(const GameState& position, const SearchStackState* ss, Move move, int change)
{
    std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
}

int16_t* CorrectionHistory::get(const GameState& position)
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey();
    return &table[stm][pawn_hash % pawn_hash_table_size];
}

void CorrectionHistory::add(const GameState& position, int, int eval_diff)
{
    auto* entry = get(position);
    // give a higher weight to high depth observations
    auto ewa_weight = 1;
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
