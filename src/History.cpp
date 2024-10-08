#include "History.h"
#include "BitBoardDefine.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "SearchData.h"

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

int16_t* FollowmoveHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    const auto& counter = (ss - 2)->move;

    if (counter == Move::Uninitialized)
    {
        return nullptr;
    }

    const auto& stm = position.Board().stm;
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto counter_piece = GetPieceType((ss - 2)->moved_piece);

    return &table[stm][counter_piece][counter.GetTo()][curr_piece][move.GetTo()];
}

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
