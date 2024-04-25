#include "GameState.h"
#include "BitBoardDefine.h"
#include "BoardState.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <memory>
#include <sstream>

#include "Move.h"
#include "MoveGeneration.h"
#include "Zobrist.h"

GameState::GameState()
{
    Reset();
    StartingPosition();
}

void GameState::ApplyMove(Move move)
{
    net.AccumulatorPush();
    previousStates.push_back(previousStates.back());
    MutableBoard().ApplyMove(move, net);
}

void GameState::ApplyMove(const std::string& strmove)
{
    Square from = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
    Square to = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
    MoveFlag flag = Board().GetMoveFlag(from, to);

    // Promotion
    if (strmove.length() == 5) // promotion: c7c8q or d2d1n	etc.
    {
        if (tolower(strmove[4]) == 'n')
            flag = (flag == CAPTURE ? KNIGHT_PROMOTION_CAPTURE : KNIGHT_PROMOTION);
        if (tolower(strmove[4]) == 'r')
            flag = (flag == CAPTURE ? ROOK_PROMOTION_CAPTURE : ROOK_PROMOTION);
        if (tolower(strmove[4]) == 'q')
            flag = (flag == CAPTURE ? QUEEN_PROMOTION_CAPTURE : QUEEN_PROMOTION);
        if (tolower(strmove[4]) == 'b')
            flag = (flag == CAPTURE ? BISHOP_PROMOTION_CAPTURE : BISHOP_PROMOTION);
    }

    ApplyMove(Move(from, to, flag));
    net.Recalculate(Board());
}

void GameState::RevertMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
    net.AccumulatorPop();
}

void GameState::ApplyNullMove()
{
    previousStates.push_back(previousStates.back());

    MutableBoard().ApplyNullMove();
}

void GameState::RevertNullMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
}

void GameState::StartingPosition()
{
    InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
}

bool GameState::InitialiseFromFen(std::vector<std::string> fen)
{
    Reset();

    if (fen.size() == 4)
    {
        fen.push_back("0");
        fen.push_back("1");
    }

    if (fen.size() < 6)
        return false; // bad fen

    bool ret = MutableBoard().InitialiseFromFen(fen);
    net.Recalculate(Board());
    return ret;
}

bool GameState::InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle,
    const std::string& ep, const std::string& fiftyMove, const std::string& turnCount)
{
    std::vector<std::string> splitFen;
    splitFen.push_back(board);
    splitFen.push_back(turn);
    splitFen.push_back(castle);
    splitFen.push_back(ep);
    splitFen.push_back(fiftyMove);
    splitFen.push_back(turnCount);

    return InitialiseFromFen(splitFen);
}

bool GameState::InitialiseFromFen(std::string fen)
{
    std::vector<std::string> splitFen; // Split the line into an array of strings seperated by each space
    std::istringstream iss(fen);
    splitFen.push_back("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    splitFen.push_back("w");
    splitFen.push_back("-");
    splitFen.push_back("-");
    splitFen.push_back("0");
    splitFen.push_back("1");

    for (int i = 0; i < 6 && iss; i++)
    {
        std::string stub;
        iss >> stub;
        if (!stub.empty())
            splitFen[i] = (stub);
    }

    return InitialiseFromFen(splitFen[0], splitFen[1], splitFen[2], splitFen[3], splitFen[4], splitFen[5]);
}

void GameState::Reset()
{
    previousStates = { BoardState() };
    net.Recalculate(Board());
}

Score GameState::GetEvaluation() const
{
    return net.Eval(Board().stm);
}

bool GameState::CheckForRep(int distanceFromRoot, int maxReps) const
{
    int totalRep = 1;
    uint64_t current = Board().GetZobristKey();

    // check every second key from the back, skipping the last (current) key
    for (int i = static_cast<int>(previousStates.size()) - 3; i >= 0; i -= 2)
    {
        if (previousStates[i].GetZobristKey() == current)
            totalRep++;

        if (totalRep == maxReps)
            return true; // maxReps (usually 3) reps is always a draw
        if (totalRep == 2 && static_cast<int>(previousStates.size() - i) < distanceFromRoot)
            return true; // Don't allow 2 reps if its in the local search history (not part of the actual played game)

        // the fifty move count is reset when a irreversible move is made. As such, we can stop here
        // and know no repitition has taken place. Becuase we move by two at a time, we stop at 0 or 1.
        if (previousStates[i].fifty_move_count <= 1)
            break;
    }

    return false;
}

const BoardState& GameState::Board() const
{
    return previousStates.back();
}

BoardState& GameState::MutableBoard()
{
    return previousStates.back();
}
