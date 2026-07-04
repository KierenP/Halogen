#include "search/staged_movegen.h"

#include "chessboard/game_state.h"
#include "movegen/list.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/data.h"
#include "search/score.h"
#include "search/static_exchange_evaluation.h"
#include "spsa/tuneable.h"

#include "simd/define.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>

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

// Equivalent to std::max_element: returns an iterator to the first element with the maximum score. The scan loop in
// selection_sort dominates move ordering cost, so we vectorize it: pass 1 finds the maximum score, pass 2 finds the
// first element equal to it. Ties resolve to the first occurrence, so the result is identical to std::max_element.
//
// Each 128-bit vector holds two ExtendedMoves; the score sits in the odd int32 lane of each pair. We use SSE4 directly
// rather than the widest available vector: the move lists are short enough that a 128-bit window beats wider vectors.
#if defined(USE_SSE4)
ExtendedMoveList::iterator first_max_element(ExtendedMoveList::iterator begin, ExtendedMoveList::iterator end)
{
    static_assert(sizeof(ExtendedMove) == 8);
    static_assert(offsetof(ExtendedMove, score) == 4);

    constexpr size_t entries = sizeof(__m128i) / sizeof(ExtendedMove); // ExtendedMove entries per vector

    const auto n = static_cast<size_t>(end - begin);
    if (n < entries)
    {
        return std::max_element(begin, end);
    }

    const auto floor = _mm_set1_epi32(INT32_MIN);
    // -1 (keep) over the odd score lanes, 0 (replace with floor) over the even move + padding lanes.
    const auto score_mask = _mm_set_epi32(-1, 0, -1, 0);

    auto vmax = floor;
    size_t i = 0;
    for (; i + entries <= n; i += entries)
    {
        const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(begin + i)));
        vmax = _mm_max_epi32(vmax, _mm_blendv_epi8(floor, v, score_mask));
    }

    const auto hi64 = _mm_shuffle_epi32(vmax, _MM_SHUFFLE(1, 0, 3, 2));
    const auto max64 = _mm_max_epi32(vmax, hi64);
    const auto hi32 = _mm_shuffle_epi32(max64, _MM_SHUFFLE(2, 3, 0, 1));
    int32_t max_score = _mm_cvtsi128_si32(_mm_max_epi32(max64, hi32));
    for (auto it = begin + i; it != end; ++it)
    {
        max_score = std::max(max_score, it->score);
    }

    const auto target = _mm_set1_epi32(max_score);
    for (i = 0; i + entries <= n; i += entries)
    {
        const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(begin + i)));
        const unsigned mask = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(v, target))) & 0b1010u;
        if (mask != 0)
        {
            return begin + i + (std::countr_zero(mask) >> 1);
        }
    }

    for (auto it = begin + i;; ++it)
    {
        if (it->score == max_score)
        {
            return it;
        }
    }
}
#else
ExtendedMoveList::iterator first_max_element(ExtendedMoveList::iterator begin, ExtendedMoveList::iterator end)
{
    return std::max_element(begin, end);
}
#endif

void selection_sort(
    ExtendedMoveList::iterator begin, ExtendedMoveList::iterator sort_end, ExtendedMoveList::iterator end)
{
    for (auto it = begin; it != sort_end; ++it)
    {
        std::iter_swap(it, first_max_element(it, end));
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

void StagedMoveGenerator::update_quiet_history(
    const Move& move, Fraction<64> positive_adjustment, Fraction<64> negative_adjustment) const
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

void StagedMoveGenerator::update_loud_history(
    const Move& move, Fraction<64> positive_adjustment, Fraction<64> negative_adjustment) const
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
            loudMoves.emplace_back(moves[i], local.get_loud_history(ss, moves[i]));
        }
    }
}
