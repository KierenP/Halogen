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
#include "spsa/tuneable.h"

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

StagedMoveGenerator StagedMoveGenerator::probcut(const GameState& position, const SearchStackState* ss,
    SearchLocalState& local, Move tt_move, Score probcut_see_margin_)
{
    auto gen = StagedMoveGenerator(position, ss, local, tt_move, false);
    gen.stage = Stage::PROBCUT_TT_MOVE;
    gen.probcut_see_margin = probcut_see_margin_;
    return gen;
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
    if (stage == Stage::PROBCUT_TT_MOVE)
    {
        stage = Stage::PROBCUT_GEN_LOUD;

        if ((TTmove.is_capture() || TTmove.is_promotion()) && is_legal(position.board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::PROBCUT_GEN_LOUD)
    {
        BasicMoveList moves;
        loud_moves(position.board(), moves);
        score_loud_moves(moves);
        current = loudMoves.begin();
        selection_sort(current, loudMoves.end(), loudMoves.end());
        stage = Stage::PROBCUT_GIVE_GOOD_LOUD;
    }

    if (stage == Stage::PROBCUT_GIVE_GOOD_LOUD)
    {
        while (current != loudMoves.end() && see_ge(position.board(), current->move, probcut_see_margin))
        {
            move = current->move;
            ++current;
            return true;
        }

        return false;
    }

    if (stage == Stage::TT_MOVE)
    {
        stage = Stage::GEN_LOUD;

        if ((!good_loud_only || TTmove.is_capture() || TTmove.is_promotion()) && is_legal(position.board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        BasicMoveList moves;
        loud_moves(position.board(), moves);
        score_loud_moves(moves);
        current = loudMoves.begin();
        selection_sort(current, loudMoves.end(), loudMoves.end());
        stage = Stage::GIVE_GOOD_LOUD;
    }

    if (stage == Stage::GIVE_GOOD_LOUD)
    {
        while (current != loudMoves.end())
        {
            if (see_ge(position.board(), current->move, -good_loud_see - current->score * good_loud_see_hist / 1024))
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

        stage = Stage::GEN_QUIET;
    }

    if (skipQuiets && (stage == Stage::GEN_QUIET || stage == Stage::GIVE_QUIET))
    {
        current = bad_loudMoves.begin();
        stage = Stage::GIVE_BAD_LOUD;
    }

    if (stage == Stage::GEN_QUIET)
    {
        BasicMoveList moves;
        quiet_moves(position.board(), moves);
        score_quiet_moves(moves);
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

void StagedMoveGenerator::score_quiet_moves(BasicMoveList& moves)
{
    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i] == TTmove)
        {
            continue;
        }

        // Quiet
        else
        {
            quietMoves.emplace_back(moves[i], local.get_quiet_order_history(ss, moves[i]));
        }
    }
}

void StagedMoveGenerator::score_loud_moves(BasicMoveList& moves)
{

    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i] == TTmove)
        {
            continue;
        }

        else
        {
            const auto cap_piece = moves[i].flag() == EN_PASSANT ? PAWN
                : moves[i].is_promotion() && !moves[i].is_capture()
                ? QUEEN
                : enum_to<PieceType>(position.board().get_square_piece(moves[i].to()));
            auto score = see_values[cap_piece] * 5 + local.get_loud_history(ss, moves[i]);
            loudMoves.emplace_back(moves[i], score);
        }
    }
}
