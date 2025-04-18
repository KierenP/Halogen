#include "StagedMoveGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "GameState.h"
#include "History.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "Score.h"
#include "SearchData.h"
#include "StaticExchangeEvaluation.h"

StagedMoveGenerator::StagedMoveGenerator(
    const GameState& Position, const SearchStackState* SS, SearchLocalState& Local, Move tt_move, bool Quiescence)
    : position(Position)
    , local(Local)
    , ss(SS)
    , quiescence(Quiescence)
    , TTmove(tt_move)
{
}

MoveGenerator StagedMoveGenerator::generate()
{
    if (!quiescence)
    {
        if (MoveIsLegal(position.Board(), TTmove))
        {
            co_yield TTmove;
        }
    }

    QuiescenceMoves(position.Board(), loudMoves);
    OrderLoudMoves(loudMoves);

    for (const auto& ext_move : loudMoves)
    {
        if (ext_move.move.IsPromotion() || see_ge(position.Board(), ext_move.move, 0))
        {
            co_yield ext_move.move;
        }
        else if (!quiescence)
        {
            bad_loudMoves.emplace_back(ext_move.move);
        }
    }

    if (quiescence)
    {
        co_return;
    }

    Killer1 = ss->killers[0];

    if (Killer1 != TTmove && MoveIsLegal(position.Board(), Killer1))
    {
        co_yield Killer1;
    }

    Killer2 = ss->killers[1];
    at_bad_loudMoves = true;

    if (Killer2 != TTmove && MoveIsLegal(position.Board(), Killer2))
    {
        co_yield Killer2;
    }

    for (const auto& ext_move : bad_loudMoves)
    {
        co_yield ext_move.move;
    }

    if (skipQuiets)
    {
        co_return;
    }

    QuietMoves(position.Board(), quietMoves);
    OrderQuietMoves(quietMoves);

    for (const auto& ext_move : quietMoves)
    {
        co_yield ext_move.move;

        if (skipQuiets)
        {
            co_return;
        }
    }
}

void StagedMoveGenerator::AdjustQuietHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.quiet_history.add(position, ss, move, positive_adjustment);
    local.cont_hist.add(position, ss, move, positive_adjustment);

    for (auto const& m : quietMoves)
    {
        if (m.move == move)
            break;

        local.quiet_history.add(position, ss, m.move, negative_adjustment);
        local.cont_hist.add(position, ss, m.move, negative_adjustment);
    }

    for (auto const& m : loudMoves)
    {
        local.loud_history.add(position, ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::AdjustLoudHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.loud_history.add(position, ss, move, positive_adjustment);

    for (auto const& m : loudMoves)
    {
        if (m.move == move)
            break;

        local.loud_history.add(position, ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::SkipQuiets()
{
    skipQuiets = true;
}

void selection_sort(ExtendedMoveList& v)
{
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        std::iter_swap(it, std::max_element(it, v.end()));
    }
}

void StagedMoveGenerator::OrderQuietMoves(ExtendedMoveList& moves)
{
    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i].move == TTmove)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Killers
        else if (moves[i].move == Killer1)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        else if (moves[i].move == Killer2)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Quiet
        else
        {
            int history = local.quiet_history.get(position, ss, moves[i].move)
                + local.cont_hist.get(position, ss, moves[i].move);
            moves[i].score
                = std::clamp<int>(history, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
        }
    }

    selection_sort(moves);
}

void StagedMoveGenerator::OrderLoudMoves(ExtendedMoveList& moves)
{
    static constexpr int16_t SCORE_QUEEN_PROMOTION = 30000;
    static constexpr int16_t SCORE_UNDER_PROMOTION = -30000;

    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i].move == TTmove)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Promotions
        else if (moves[i].move.IsPromotion())
        {
            if (moves[i].move.GetFlag() == QUEEN_PROMOTION || moves[i].move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
            {
                moves[i].score = SCORE_QUEEN_PROMOTION;
            }
            else
            {
                moves[i].score = SCORE_UNDER_PROMOTION;
            }
        }

        // Captures
        else
        {
            int history = local.loud_history.get(position, ss, moves[i].move);
            moves[i].score
                = std::clamp<int>(history, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
        }
    }

    selection_sort(moves);
}
