#include "chessboard/game_state.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>

#include "Move.h"
#include "Zobrist.h"
#include "bitboard.h"
#include "chessboard/board_state.h"
#include "search/cuckoo.h"

void GameState::apply_move(Move move)
{
    previousStates.emplace_back(previousStates.back()).apply_move(move);
    update_current_position_repetition();
}

void GameState::apply_move(std::string_view strmove)
{
    Square from = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
    Square to = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
    MoveFlag flag = board().infer_move_flag(from, to);

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
        to = lsb(board().castle_squares & RankBB[board().stm == WHITE ? RANK_1 : RANK_8]);
    }
    else if (flag == H_SIDE_CASTLE)
    {
        to = msb(board().castle_squares & RankBB[board().stm == WHITE ? RANK_1 : RANK_8]);
    }

    apply_move(Move(from, to, flag));

    // trim all moves before a 50 move reset
    if (board().fifty_move_count == 0)
    {
        previousStates.erase(previousStates.begin(), previousStates.end() - 1);
    }
}

void GameState::revert_move()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
}

void GameState::apply_null_move()
{
    previousStates.push_back(previousStates.back());
    MutableBoard().apply_null_move();
    update_current_position_repetition();
}

void GameState::revert_null_move()
{
    assert(previousStates.size() > 0);

    previousStates.pop_back();
}

GameState GameState::starting_position()
{
    return from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

GameState GameState::from_fen(std::string_view fen)
{
    GameState state;
    state.init_from_fen(fen);
    return state;
}

bool GameState::init_from_fen(std::array<std::string_view, 6> fen)
{
    previousStates.clear();
    return previousStates.emplace_back().init_from_fen(fen);
}

bool GameState::init_from_fen(std::string_view fen)
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

    return init_from_fen(splitFen);
}

const BoardState& GameState::board() const
{
    assert(previousStates.size() >= 1);
    return previousStates.back();
}

const BoardState& GameState::prev_board() const
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
    return board().three_fold_rep
        || (board().repetition.has_value() && board().repetition.value() < distance_from_root);
}

bool GameState::is_two_fold_repetition() const
{
    return board().repetition.has_value();
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
        if (previousStates[i].key == previousStates[i - ply].key)
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

    uint64_t other = previousStates[i].key ^ previousStates[i - 1].key ^ Zobrist::stm();
    uint64_t occ = previousStates[i].get_pieces_bb();

    for (int ply = 3; ply <= max_ply; ply += 2)
    {
        other ^= previousStates[i - (ply - 1)].key ^ previousStates[i - ply].key ^ Zobrist::stm();

        if (other != 0)
        {
            // Opponent pieces must have reverted
            continue;
        }

        // 'diff' is a single move
        uint64_t diff = previousStates[i].key ^ previousStates[i - ply].key;
        auto hash = Cuckoo::H1(diff);

        if (Cuckoo::table[hash] == diff || (hash = Cuckoo::H2(diff), Cuckoo::table[hash] == diff))
        {
            const auto move = Cuckoo::move_table[hash];
            if ((occ & BetweenBB[move.GetFrom()][move.GetTo()]) == EMPTY)
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
