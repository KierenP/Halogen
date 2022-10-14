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
    moveStack.push_back(move);
    net.AccumulatorPush();
    previousStates.push_back(previousStates.back());
    MutableBoard().ApplyMove(move, net);
}

void GameState::ApplyMove(const std::string& strmove)
{
    Square prev = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
    Square next = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
    MoveFlag flag = QUIET;

    //Captures
    if (Board().IsOccupied(next))
        flag = CAPTURE;

    //Double pawn moves
    if (AbsRankDiff(prev, next) == 2)
        if (Board().GetSquare(prev) == WHITE_PAWN || Board().GetSquare(prev) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    //En passant
    if (next == Board().en_passant)
        if (Board().GetSquare(prev) == WHITE_PAWN || Board().GetSquare(prev) == BLACK_PAWN)
            flag = EN_PASSANT;

    //Castling (normal chess)
    if (prev == SQ_E1 && next == SQ_G1 && Board().GetSquare(prev) == WHITE_KING)
        flag = H_SIDE_CASTLE;

    if (prev == SQ_E1 && next == SQ_C1 && Board().GetSquare(prev) == WHITE_KING)
        flag = A_SIDE_CASTLE;

    if (prev == SQ_E8 && next == SQ_G8 && Board().GetSquare(prev) == BLACK_KING)
        flag = H_SIDE_CASTLE;

    if (prev == SQ_E8 && next == SQ_C8 && Board().GetSquare(prev) == BLACK_KING)
        flag = A_SIDE_CASTLE;

    //Promotion
    if (strmove.length() == 5) //promotion: c7c8q or d2d1n	etc.
    {
        if (tolower(strmove[4]) == 'n')
            flag = KNIGHT_PROMOTION;
        if (tolower(strmove[4]) == 'r')
            flag = ROOK_PROMOTION;
        if (tolower(strmove[4]) == 'q')
            flag = QUEEN_PROMOTION;
        if (tolower(strmove[4]) == 'b')
            flag = BISHOP_PROMOTION;

        if (Board().IsOccupied(next)) //if it's a capture we need to shift the flag up 4 to turn it from eg: KNIGHT_PROMOTION to KNIGHT_PROMOTION_CAPTURE. EDIT: flag == capture wont work because we just changed the flag!! This was a bug back from 2018 found in 2020
            flag = static_cast<MoveFlag>(flag + CAPTURE); //might be slow, but don't care.
    }

    // Castling (chess960)
    if (Board().GetSquare(prev) == WHITE_KING && Board().GetSquare(next) == WHITE_ROOK)
    {
        if (prev > next)
        {
            flag = A_SIDE_CASTLE;
            next = SQ_C1;
        }
        else
        {
            flag = H_SIDE_CASTLE;
            next = SQ_G1;
        }
    }

    if (Board().GetSquare(prev) == BLACK_KING && Board().GetSquare(next) == BLACK_ROOK)
    {
        if (prev > next)
        {
            flag = A_SIDE_CASTLE;
            next = SQ_C8;
        }
        else
        {
            flag = H_SIDE_CASTLE;
            next = SQ_G8;
        }
    }

    ApplyMove(Move(prev, next, flag));
    net.Recalculate(Board());
}

void GameState::RevertMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
    net.AccumulatorPop();
    moveStack.pop_back();
}

void GameState::ApplyNullMove()
{
    moveStack.push_back(Move::Uninitialized);
    previousStates.push_back(previousStates.back());

    MutableBoard().ApplyNullMove();
}

void GameState::RevertNullMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
    moveStack.pop_back();
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
        return false; //bad fen

    bool ret = MutableBoard().InitialiseFromFen(fen);
    net.Recalculate(Board());
    return ret;
}

bool GameState::InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle, const std::string& ep, const std::string& fiftyMove, const std::string& turnCount)
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
    std::vector<std::string> splitFen; //Split the line into an array of strings seperated by each space
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
    moveStack.clear();
    previousStates = { BoardState() };
    net.Recalculate(Board());
}

int16_t GameState::GetEvaluation() const
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
            return true; //maxReps (usually 3) reps is always a draw
        if (totalRep == 2 && static_cast<int>(previousStates.size() - i) < distanceFromRoot)
            return true; //Don't allow 2 reps if its in the local search history (not part of the actual played game)

        // the fifty move count is reset when a irreversible move is made. As such, we can stop here
        // and know no repitition has taken place. Becuase we move by two at a time, we stop at 0 or 1.
        if (previousStates[i].fifty_move_count <= 1)
            break;
    }

    return false;
}

Move GameState::GetPreviousMove() const
{
    if (moveStack.size())
        return moveStack.back();
    else
        return Move::Uninitialized;
}

const BoardState& GameState::Board() const
{
    return previousStates.back();
}

BoardState& GameState::MutableBoard()
{
    return previousStates.back();
}
