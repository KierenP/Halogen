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
    const auto counter_piece = GetPieceType(position.Board().GetSquare(counter.GetTo()));

    return &table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

int16_t* CaptureHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto cap_piece = move.GetFlag() == EN_PASSANT ? PAWN : GetPieceType(position.Board().GetSquare(move.GetTo()));

    return &table[stm][curr_piece][move.GetTo()][cap_piece];
}
