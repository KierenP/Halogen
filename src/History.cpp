#include "History.h"
#include "BitBoardDefine.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "SearchData.h"

int16_t* PawnHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    const auto pawn_hash = position.Board().GetPawnKey() % pawn_states;
    // TODO: could use ss->moved_piece?
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
    // TODO: could use ss->moved_piece?
    const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));
    const auto cap_piece = move.GetFlag() == EN_PASSANT ? PAWN : GetPieceType(position.Board().GetSquare(move.GetTo()));

    return &table[stm][curr_piece][move.GetTo()][cap_piece];
}

int16_t* PieceMoveHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    // TODO: could use ss->moved_piece?
    const auto piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

    return &table[stm][piece][move.GetTo()];
}

std::array<PieceMoveHistory*, ContinuationHistory::cont_hist_depth> ContinuationHistory::get_subtables(
    const SearchStackState* ss)
{
    std::array<PieceMoveHistory*, cont_hist_depth> subtables;

    for (int i = 0; i < cont_hist_depth; i++)
    {
        subtables[i] = (ss - i - 1)->cont_hist_subtable;
    }

    return subtables;
}

void ContinuationHistory::add(const GameState& position, const SearchStackState* ss, Move move, int change)
{
    for (auto* cont_hist_table : ss->cont_hist_subtables)
    {
        if (!cont_hist_table)
        {
            continue;
        }

        cont_hist_table->add(position, ss, move, change);
    }
}

int32_t ContinuationHistory::get(const GameState& position, const SearchStackState* ss, Move move)
{
    int32_t sum = 0;

    for (auto* cont_hist_table : ss->cont_hist_subtables)
    {
        if (!cont_hist_table)
        {
            continue;
        }

        sum += *cont_hist_table->get(position, ss, move);
    }

    return sum;
}
