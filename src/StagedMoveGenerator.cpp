#include "StagedMoveGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

#include "BitBoardDefine.h"
#include "GameState.h"
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
    moveSEE = std::nullopt;

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
        OrderMoves(loudMoves);
        current = loudMoves.begin();
        stage = Stage::GIVE_GOOD_LOUD;
    }

    if (stage == Stage::GIVE_GOOD_LOUD)
    {
        if (current != loudMoves.end() && current->SEE >= 0)
        {
            move = current->move;
            moveSEE = current->SEE;
            ++current;
            return true;
        }

        if (quiescence)
            return false;

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
        if (current != loudMoves.end())
        {
            move = current->move;
            moveSEE = current->SEE;
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
        OrderMoves(quietMoves);
        current = quietMoves.begin();
        stage = Stage::GIVE_QUIET;
    }

    if (stage == Stage::GIVE_QUIET)
    {
        if (current != quietMoves.end())
        {
            move = current->move;
            moveSEE = current->SEE;
            ++current;
            return true;
        }
    }

    return false;
}

void StagedMoveGenerator::AdjustHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.history.add(position, ss, move, positive_adjustment);

    for (auto const& m : quietMoves)
    {
        if (m.move == move)
            break;

        local.history.add(position, ss, m.move, negative_adjustment);
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

int16_t StagedMoveGenerator::GetSEE(Move) const
{
    return *moveSEE;
}

void StagedMoveGenerator::OrderMoves(ExtendedMoveList& moves)
{
    static constexpr int16_t SCORE_QUEEN_PROMOTION = 30000;
    static constexpr int16_t SCORE_CAPTURE = 20000;
    static constexpr int16_t SCORE_UNDER_PROMOTION = -1;

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

        // Promotions
        else if (moves[i].move.IsPromotion())
        {
            if (moves[i].move.GetFlag() == QUEEN_PROMOTION || moves[i].move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
            {
                moves[i].score = SCORE_QUEEN_PROMOTION;
                moves[i].SEE = PieceValues[QUEEN];
            }
            else
            {
                moves[i].score = SCORE_UNDER_PROMOTION;
            }
        }

        // Captures
        else if (moves[i].move.IsCapture())
        {
            int SEE = see(position.Board(), moves[i].move);
            moves[i].score = SCORE_CAPTURE + SEE;
            moves[i].SEE = SEE;
        }

        // Quiet
        else
        {
            int history = local.history.get(position, ss, moves[i].move);
            moves[i].score = std::clamp<int>(history, std::numeric_limits<decltype(moves[i].score)>::min(),
                std::numeric_limits<decltype(moves[i].score)>::max());
        }
    }

    selection_sort(moves);
}
