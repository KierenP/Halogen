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
#include "MoveList.h"
#include "Score.h"
#include "SearchData.h"
#include "StaticExchangeEvaluation.h"

StagedMoveGenerator::StagedMoveGenerator(
    const GameState& Position, const SearchStackState* SS, SearchLocalState& Local, Move tt_move, bool good_loud_only_)
    : position(Position)
    , local(Local)
    , ss(SS)
    , good_loud_only(good_loud_only_)
    , stage(Stage::TT_MOVE)
    , TTmove(tt_move)
{
}

void selection_sort(
    ExtendedMoveList::iterator begin, ExtendedMoveList::iterator sort_end, ExtendedMoveList::iterator end)
{
    for (auto it = begin; it != sort_end; ++it)
    {
        std::iter_swap(it, std::max_element(it, end));
    }
}

bool StagedMoveGenerator::Next(Move& move)
{
    if (stage == Stage::TT_MOVE)
    {
        stage = Stage::GEN_LOUD;

        if ((!good_loud_only || TTmove.IsCapture() || TTmove.IsPromotion()) && MoveIsLegal(position.Board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        QuiescenceMoves(position.Board(), loudMoves);
        ScoreLoudMoves(loudMoves);
        current = loudMoves.begin();
        selection_sort(current, loudMoves.end(), loudMoves.end());
        stage = Stage::GIVE_GOOD_LOUD;
    }

    if (stage == Stage::GIVE_GOOD_LOUD)
    {
        while (current != loudMoves.end())
        {
            if (current->move.IsPromotion() || see_ge(position.Board(), current->move, 0))
            {
                move = current->move;
                ++current;
                return true;
            }
            else
            {
                bad_loudMoves.emplace_back(*current);
                ++current;
            }
        }

        if (good_loud_only)
            return false;

        current = bad_loudMoves.begin();
        stage = Stage::GIVE_KILLER_1;
    }

    if (stage == Stage::GIVE_KILLER_1)
    {
        Killer1 = ss->killers[0];
        stage = Stage::GIVE_KILLER_2;

        if (Killer1 != TTmove && MoveIsLegal(position.Board(), Killer1))
        {
            move = Killer1;
            return true;
        }
    }

    if (stage == Stage::GIVE_KILLER_2)
    {
        Killer2 = ss->killers[1];
        stage = Stage::GIVE_BAD_LOUD;

        if (Killer2 != TTmove && MoveIsLegal(position.Board(), Killer2))
        {
            move = Killer2;
            return true;
        }
    }

    if (stage == Stage::GIVE_BAD_LOUD)
    {
        if (current != bad_loudMoves.end())
        {
            move = current->move;
            ++current;
            return true;
        }
        else
        {
            stage = Stage::GEN_QUIET;
        }
    }

    if (skipQuiets)
        return false;

    if (stage == Stage::GEN_QUIET)
    {
        QuietMoves(position.Board(), quietMoves);
        ScoreQuietMoves(quietMoves);
        current = sorted_end = quietMoves.begin();
        stage = Stage::GIVE_QUIET;
    }

    if (stage == Stage::GIVE_QUIET)
    {
        if (current != quietMoves.end())
        {
            // rather than sorting the whole list, we sort in chunks at a time
            if (current >= sorted_end)
            {
                sorted_end = std::min(sorted_end + 5, quietMoves.end());
                selection_sort(current, sorted_end, quietMoves.end());
            }

            move = current->move;
            ++current;
            return true;
        }
    }

    return false;
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

void StagedMoveGenerator::ScoreQuietMoves(ExtendedMoveList& moves)
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
}

void StagedMoveGenerator::ScoreLoudMoves(ExtendedMoveList& moves)
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
}
