#include "GameState.h"
#include "BitBoardDefine.h"
#include "BoardState.h"

#include <assert.h>
#include <ctype.h>
#include <iterator>
#include <sstream>

#include "Move.h"
#include "MoveGeneration.h"
#include "Zobrist.h"

GameState::GameState()
{
    StartingPosition();
}

void GameState::ApplyMove(Move move)
{
    auto& ss = state_stack.emplace_back();
    board.ApplyMove(move, ss);
    net.ApplyMove(board, ss, move);
    assert(net.Verify(board));
}

void GameState::ApplyMove(std::string_view strmove)
{
    Square from = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
    Square to = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
    MoveFlag flag = board.GetMoveFlag(from, to);

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
    net.Recalculate(board);
}

void GameState::RevertMove(Move move)
{
    assert(state_stack.size() > 0);

    board.RevertMove(move, state_stack.back());
    state_stack.pop_back();
    net.UndoMove();
    assert(net.Verify(board));
}

void GameState::ApplyNullMove()
{
    auto& ss = state_stack.emplace_back();
    board.ApplyNullMove(ss);
}

void GameState::RevertNullMove()
{
    assert(state_stack.size() > 0);

    board.RevertNullMove(state_stack.back());
    state_stack.pop_back();
}

void GameState::StartingPosition()
{
    InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool GameState::InitialiseFromFen(std::array<std::string_view, 6> fen)
{
    state_stack.clear();
    auto& ss = state_stack.emplace_back();
    bool ret = board.InitialiseFromFen(fen, ss);
    net.Recalculate(board);
    return ret;
}

bool GameState::InitialiseFromFen(std::string_view fen)
{
    // Split the line into an array of strings seperated by each space
    std::array<std::string_view, 6> splitFen = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
        "w",
        "-",
        "-",
        "0",
        "1",
    };

    size_t idx = 0;
    size_t str_idx = 0;

    while (str_idx < fen.size() && idx < 6)
    {
        auto next = fen.find(" ", str_idx);
        if (next == fen.npos)
        {
            next = fen.size();
        }
        splitFen[idx] = fen.substr(str_idx, next - str_idx);
        str_idx = next + 1;
        idx++;
    }

    return InitialiseFromFen(splitFen);
}

Score GameState::GetEvaluation() const
{
    return net.Eval(board);
}

bool GameState::CheckForRep(int distanceFromRoot, int maxReps) const
{
    int totalRep = 1;
    uint64_t current = board.GetZobristKey();

    // check every second key from the back. state_stack[i].key represents the key before applying move i, so the first
    // key we check is 1 from the end
    for (int i = static_cast<int>(state_stack.size()) - 2; i >= 0; i -= 2)
    {
        if (state_stack[i].key.Key() == current)
            totalRep++;

        if (totalRep == maxReps)
            return true; // maxReps (usually 3) reps is always a draw
        if (totalRep == 2 && static_cast<int>(state_stack.size() - i) + 1 < distanceFromRoot)
            return true; // Don't allow 2 reps if its in the local search history (not part of the actual played game)

        // the fifty move count is reset when a irreversible move is made. As such, we can stop here
        // and know no repitition has taken place. Becuase we move by two at a time, we stop at 0 or 1.
        if (state_stack[i].fifty_move_count <= 1)
            break;
    }

    return false;
}

const BoardState& GameState::Board() const
{
    return board;
}
