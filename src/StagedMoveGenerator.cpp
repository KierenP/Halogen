#include "StagedMoveGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

#include "BitBoardDefine.h"
#include "GameState.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "SearchData.h"
#include "StaticExchangeEvaluation.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

StagedMoveGenerator::StagedMoveGenerator(
    const GameState& Position, const SearchStackState* SS, SearchLocalState& Local, Move tt_move, bool Quiescence)
    : position(Position)
    , local(Local)
    , ss(SS)
    , quiescence(Quiescence)
    , TTmove(tt_move)
{
    if (quiescence)
        stage = Stage::GEN_LOUD;
    else
        stage = Stage::TT_MOVE;
}

bool StagedMoveGenerator::Next(Move& move)
{
    if (stage == Stage::TT_MOVE)
    {
        stage = Stage::GEN_LOUD;

        if (MoveIsLegal(position.Board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        QuiescenceMoves(position.Board(), loudMoves);
        OrderLoudMoves(loudMoves);
        current = loudMoves.begin();
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

        if (quiescence)
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
        OrderQuietMoves(quietMoves);
        current = quietMoves.begin();
        stage = Stage::GIVE_QUIET;
    }

    if (stage == Stage::GIVE_QUIET)
    {
        if (current != quietMoves.end())
        {
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

    for (auto const& m : quietMoves)
    {
        if (m.move == move)
            break;

        local.quiet_history.add(position, ss, m.move, negative_adjustment);
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
            int history = local.quiet_history.get(position, ss, moves[i].move);
            moves[i].score = std::clamp<int>(history, std::numeric_limits<decltype(moves[i].score)>::min(),
                std::numeric_limits<decltype(moves[i].score)>::max());
        }
    }

    selection_sort(moves);
}

void StagedMoveGenerator::OrderLoudMoves(ExtendedMoveList& moves)
{
    static constexpr int16_t SCORE_QUEEN_PROMOTION = 30000;
    static constexpr int16_t SCORE_UNDER_PROMOTION = -30000;

    static constexpr std::array MVV_LVA = {
        std::array { 15, 14, 13, 12, 11, 10 },
        std::array { 25, 24, 23, 22, 21, 20 },
        std::array { 35, 34, 33, 32, 31, 30 },
        std::array { 45, 44, 43, 42, 41, 40 },
        std::array { 55, 54, 53, 52, 51, 50 },
        std::array { 0, 0, 0, 0, 0, 0 },
    };

    auto capture_score = [](auto mvv_lva, auto history)
    {
        auto score = (mvv_lva - 30) * 500 + history;
        return std::clamp<int>(score, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
    };

    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i].move == TTmove)
        {
            moves.erase(moves.begin() + i);
            i--;
            continue;
        }

        // Promotions
        if (moves[i].move.IsPromotion())
        {
            if (moves[i].move.GetFlag() == QUEEN_PROMOTION || moves[i].move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
            {
                moves[i].score = SCORE_QUEEN_PROMOTION;
            }
            else
            {
                moves[i].score = SCORE_UNDER_PROMOTION;
            }

            continue;
        }

        // Handle en passant separate to MVV_LVA
        if (moves[i].move.GetFlag() == EN_PASSANT)
        {
            auto mvv_lva = MVV_LVA[PAWN][PAWN];
            auto history = local.quiet_history.get(position, ss, moves[i].move);
            moves[i].score = capture_score(mvv_lva, history);
        }

        // Captures
        else
        {
            auto mvv_lva = MVV_LVA[GetPieceType(position.Board().GetSquare(moves[i].move.GetTo()))]
                                  [GetPieceType(position.Board().GetSquare(moves[i].move.GetFrom()))];
            auto history = local.quiet_history.get(position, ss, moves[i].move);
            moves[i].score = capture_score(mvv_lva, history);
        }
    }

    selection_sort(moves);
}
