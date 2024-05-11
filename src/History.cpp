#include "History.h"
#include "BitBoardDefine.h"
#include "Move.h"
#include "SearchData.h"

int16_t& ButterflyHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    return table[stm][move.GetFrom()][move.GetTo()];
}

bool ButterflyHistory::valid(const GameState&, const SearchStackState*, Move move)
{
    return !move.IsCapture() && !move.IsPromotion();
}

int16_t& CountermoveHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 1)->move;
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto counter_piece = GetPieceType(position.Board().GetSquare(counter.GetTo()));

    return table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

bool CountermoveHistory::valid(const GameState&, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 1)->move;
    return !move.IsCapture() && !move.IsPromotion() && counter != Move::Uninitialized;
}

int16_t& CaptureHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto capture_piece
        = move.GetFlag() == EN_PASSANT ? PAWN : GetPieceType(position.Board().GetSquare(move.GetTo()));

    return table[stm][curr_piece][move.GetTo()][capture_piece];
}

bool CaptureHistory::valid(const GameState&, const SearchStackState*, Move move)
{
    return move.IsCapture();
}

void History::reset()
{
    std::apply([](auto&... table) { (table.reset(), ...); }, tables_);
}

int History::get(const GameState& position, const SearchStackState* ss, Move move)
{
    int applied_histories = 0;
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
    return applied_histories == 0 ? 0 : history_sum / applied_histories;
}

void History::add(const GameState& position, const SearchStackState* ss, Move move, int change)
{
    std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
}
