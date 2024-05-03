#include "History.h"
#include "BitBoardDefine.h"
#include "SearchData.h"

int16_t* ButterflyHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;

    if (move != Move::Uninitialized)
    {
        return &table[stm][move.GetFrom()][move.GetTo()];
    }
    else
    {
        return nullptr;
    }
}

int16_t* CountermoveHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 1)->move;
    const auto& stm = position.Board().stm;

    const auto& curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto& counter_piece = GetPieceType(position.Board().GetSquare(counter.GetTo()));

    if (move != Move::Uninitialized && counter != Move::Uninitialized && curr_piece != N_PIECE_TYPES
        && counter_piece != N_PIECE_TYPES)
    {
        return &table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
    }
    else
    {
        return nullptr;
    }
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

    return std::apply([&](auto&... table) { return (get_value(table) + ...); }, tables_);
}

void History::add(const GameState& position, const SearchStackState* ss, Move move, int change)
{
    std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
}
