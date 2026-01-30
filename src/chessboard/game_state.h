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
    [[nodiscard]] static GameState starting_position() noexcept;
    [[nodiscard]] static GameState from_fen(std::string_view fen) noexcept;

    void apply_move(Move move) noexcept;
    void apply_move(std::string_view strmove) noexcept;
    void revert_move() noexcept;

    void apply_null_move() noexcept;
    void revert_null_move() noexcept;

    // returns true after sucsessful execution, false otherwise
    bool init_from_fen(std::string_view fen) noexcept;

    bool is_repetition(int distance_from_root) const noexcept;
    bool is_two_fold_repetition() const noexcept;
    bool upcoming_rep(int distanceFromRoot, Move excluded_move = Move::Uninitialized) const noexcept;
    bool has_repeated() const noexcept;

    const BoardState& board() const noexcept;
    const BoardState& prev_board() const noexcept;

private:
    GameState() = default;
    BoardState& MutableBoard() noexcept;

    // We store history back to the last zeroing move, and enough space for the max search depth
    StaticVector<BoardState, 100 + MAX_RECURSION + 1> previousStates;

    bool init_from_fen(std::array<std::string_view, 6> fen) noexcept;
    void update_current_position_repetition() noexcept;
};
