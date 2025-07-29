#include "search/staged_movegen.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "chessboard/game_state.h"
#include "movegen/list.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/data.h"
#include "search/history.h"
#include "search/score.h"
#include "search/static_exchange_evaluation.h"

StagedMoveGenerator::StagedMoveGenerator(
    const GameState& Position, const SearchStackState* SS, SearchLocalState& Local, Move tt_move, bool Quiescence)
    : position(Position)
    , local(Local)
    , ss(SS)
    , quiescence(Quiescence)
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

bool StagedMoveGenerator::next(Move& move)
{
    if (stage == Stage::TT_MOVE)
    {
        stage = Stage::GEN_LOUD;

        if ((!quiescence || TTmove.is_capture() || TTmove.is_promotion()) && is_legal(position.board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        loud_moves(position.board(), loudMoves);
        score_loud_moves(loudMoves);
        current = loudMoves.begin();
        selection_sort(current, loudMoves.end(), loudMoves.end());
        stage = Stage::GIVE_GOOD_LOUD;
    }

    if (stage == Stage::GIVE_GOOD_LOUD)
    {
        while (current != loudMoves.end())
        {
            if (see_ge(position.board(), current->move, 0))
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

        if (quiescence)
            return false;

        stage = Stage::GEN_QUIET;
    }

    if (skipQuiets && (stage == Stage::GEN_QUIET || stage == Stage::GIVE_QUIET))
    {
        current = bad_loudMoves.begin();
        stage = Stage::GIVE_BAD_LOUD;
    }

    if (stage == Stage::GEN_QUIET)
    {
        quiet_moves(position.board(), quietMoves);
        score_quiet_moves(quietMoves);
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

        current = bad_loudMoves.begin();
        stage = Stage::GIVE_BAD_LOUD;
    }

    if (stage == Stage::GIVE_BAD_LOUD)
    {
        if (current != bad_loudMoves.end())
        {
            move = current->move;
            ++current;
            return true;
        }
    }

    return false;
}

void StagedMoveGenerator::update_quiet_history(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.add_quiet_history(ss, move, positive_adjustment);

    for (auto const& m : quietMoves)
    {
        if (m.move == move)
            break;

        local.add_quiet_history(ss, m.move, negative_adjustment);
    }

    for (auto const& m : loudMoves)
    {
        if (!bad_loudMoves.empty() && m.move == bad_loudMoves.front().move)
            break;

        local.add_loud_history(ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::update_loud_history(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.add_loud_history(ss, move, positive_adjustment);

    for (auto const& m : loudMoves)
    {
        if (m.move == move)
            break;

        local.add_loud_history(ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::skip_quiets()
{
    skipQuiets = true;
}

void StagedMoveGenerator::score_quiet_moves(ExtendedMoveList& moves)
{
    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i].move == TTmove)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Quiet
        else
        {
            moves[i].score = local.get_quiet_order_history(ss, moves[i].move);
        }
    }
}

void StagedMoveGenerator::score_loud_moves(ExtendedMoveList& moves)
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
        else if (moves[i].move.is_promotion())
        {
            if (moves[i].move.flag() == QUEEN_PROMOTION || moves[i].move.flag() == QUEEN_PROMOTION_CAPTURE)
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
            moves[i].score = local.get_loud_history(ss, moves[i].move);
        }
    }
}
