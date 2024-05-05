#include "History.h"
#include "BitBoardDefine.h"
#include "SearchData.h"

int16_t& ButterflyHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    return table[stm][move.GetFrom()][move.GetTo()];
}

bool ButterflyHistory::valid(const GameState&, const SearchStackState*, Move)
{
    return true;
}

int16_t& CountermoveHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 1)->move;
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto counter_piece = GetPieceType(position.Board().GetSquare(counter.GetTo()));

    return table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

bool CountermoveHistory::valid(const GameState&, const SearchStackState* ss, Move)
{
    const auto& counter = (ss - 1)->move;
    return counter != Move::Uninitialized;
}

void History::reset()
{
    std::apply([](auto&... table) { (table.reset(), ...); }, tables_);
}

int History::get(const GameState& position, const SearchStackState* ss, Move move)
{
    size_t applied_histories = 0;
    int history_sum = 0;
    auto sum_history = [&](auto& table)
    {
        if (table.valid(position, ss, move))
        {
            applied_histories++;
            history_sum += table.get(position, ss, move);
        }
    };

    std::apply([&](auto&... table) { return (sum_history(table), ...); }, tables_);
    return history_sum / applied_histories;
}

void History::add(const GameState& position, const SearchStackState* ss, Move move, int change)
{
    std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
}
