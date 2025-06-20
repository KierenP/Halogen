#include "GameState.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Cuckoo.h"
#include "Move.h"
#include "Zobrist.h"

GameState::GameState()
{
    StartingPosition();
}

void GameState::ApplyMove(Move move)
{
    previousStates.emplace_back(previousStates.back()).ApplyMove(move);
    update_current_position_repetition();
}

void GameState::ApplyMove(std::string_view strmove)
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

    // Correction for castle moves (encode as KxR)
    if (flag == A_SIDE_CASTLE)
    {
        to = LSB(Board().castle_squares & RankBB[Board().stm == WHITE ? RANK_1 : RANK_8]);
    }
    else if (flag == H_SIDE_CASTLE)
    {
        to = MSB(Board().castle_squares & RankBB[Board().stm == WHITE ? RANK_1 : RANK_8]);
    }

    ApplyMove(Move(from, to, flag));

    // trim all moves before a 50 move reset
    if (Board().fifty_move_count == 0)
    {
        previousStates.erase(previousStates.begin(), previousStates.end() - 1);
    }
}

void GameState::RevertMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
}

void GameState::ApplyNullMove()
{
    previousStates.push_back(previousStates.back());
    MutableBoard().ApplyNullMove();
    update_current_position_repetition();
}

void GameState::RevertNullMove()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
}

void GameState::StartingPosition()
{
    InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

bool GameState::InitialiseFromFen(std::array<std::string_view, 6> fen)
{
    previousStates.clear();
    return previousStates.emplace_back().InitialiseFromFen(fen);
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

const BoardState& GameState::Board() const
{
    assert(previousStates.size() >= 1);
    return previousStates.back();
}

const BoardState& GameState::PrevBoard() const
{
    assert(previousStates.size() >= 2);
    return previousStates[previousStates.size() - 2];
}

BoardState& GameState::MutableBoard()
{
    assert(previousStates.size() >= 1);
    return previousStates.back();
}

bool GameState::is_repetition(int distance_from_root) const
{
    return Board().three_fold_rep
        || (Board().repetition.has_value() && Board().repetition.value() < distance_from_root);
}

bool GameState::is_two_fold_repetition() const
{
    return Board().repetition.has_value();
}

void GameState::update_current_position_repetition()
{
    assert(previousStates.size() >= 1);

    const int i = (int)previousStates.size() - 1;
    previousStates[i].three_fold_rep = false;
    previousStates[i].repetition = std::nullopt;

    const int max_ply = std::min(i, previousStates[i].fifty_move_count);

    for (int ply = 4; ply <= max_ply; ply += 2)
    {
        if (previousStates[i].GetZobristKey() == previousStates[i - ply].GetZobristKey())
        {
            previousStates[i].repetition = ply;
            previousStates[i].three_fold_rep = (bool)previousStates[i - ply].repetition;
            break;
        }
    }
}

bool GameState::upcoming_rep(int distanceFromRoot) const
{
    const int i = (int)previousStates.size() - 1;
    const int max_ply = std::min(i, previousStates[i].fifty_move_count);

    // Enough reversible moves played
    if (max_ply < 3)
    {
        return false;
    }

    uint64_t other = previousStates[i].GetZobristKey() ^ previousStates[i - 1].GetZobristKey() ^ Zobrist::stm();
    uint64_t occ = previousStates[i].GetAllPieces();

    for (int ply = 3; ply <= max_ply; ply += 2)
    {
        other
            ^= previousStates[i - (ply - 1)].GetZobristKey() ^ previousStates[i - ply].GetZobristKey() ^ Zobrist::stm();

        if (other != 0)
        {
            // Opponent pieces must have reverted
            continue;
        }

        // 'diff' is a single move
        uint64_t diff = previousStates[i].GetZobristKey() ^ previousStates[i - ply].GetZobristKey();
        auto hash = cuckoo::H1(diff);

        if (cuckoo::table[hash] == diff || (hash = cuckoo::H2(diff), cuckoo::table[hash] == diff))
        {
            const auto move = cuckoo::move_table[hash];
            if ((occ & betweenArray[move.GetFrom()][move.GetTo()]) == EMPTY)
            {
                // two fold rep after root
                if (ply < distanceFromRoot)
                {
                    return true;
                }

                // three fold rep
                if (previousStates[i - ply].repetition)
                {
                    return true;
                }
            }
        }
    }

    return false;
}
