#pragma once

#include <array>
#include <string_view>
#include <vector>

#include "chessboard/board_state.h"

class Move;

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
    bool upcoming_rep(int distanceFromRoot) const;

    const BoardState& board() const;
    const BoardState& prev_board() const;

private:
    GameState() = default;
    BoardState& MutableBoard();

    std::vector<BoardState> previousStates;

    bool init_from_fen(std::array<std::string_view, 6> fen);
    void update_current_position_repetition();
};
