#pragma once

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "utility/static_vector.h"

#include <array>
#include <string_view>

/*
This class holds all the data required to define a state in a chess game,
including all previous game states for the purposes of draw by repetition.
*/

class GameState
{
public:
    [[nodiscard]] static GameState starting_position();
    [[nodiscard]] static GameState from_fen(std::string_view fen);

    void apply_move(Move move);
    void apply_move(std::string_view strmove);
    void revert_move();

    void apply_null_move();
    void revert_null_move();

    // returns true after sucsessful execution, false otherwise
    bool init_from_fen(std::string_view fen);

    bool is_repetition(int distance_from_root) const;
    bool is_two_fold_repetition() const;
    bool upcoming_rep(int distanceFromRoot, Move excluded_move = Move::Uninitialized) const;
    bool has_repeated() const;

    const BoardState& board() const;
    const BoardState& prev_board() const;

private:
    GameState() = default;
    BoardState& MutableBoard();

    // We store history back to the last zeroing move, and enough space for the max search depth
    StaticVector<BoardState, 100 + MAX_RECURSION + 1> previousStates;

    bool init_from_fen(std::array<std::string_view, 6> fen);
    void update_current_position_repetition();
};
