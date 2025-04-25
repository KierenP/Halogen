#pragma once

#include <array>
#include <string_view>
#include <vector>

#include "BoardState.h"

class Move;

/*
This class holds all the data required to define a state in a chess game,
including all previous game states for the purposes of draw by repitition.
*/

class GameState
{
public:
    GameState();

    void ApplyMove(Move move);
    void ApplyMove(std::string_view strmove);
    void RevertMove();

    void ApplyNullMove();
    void RevertNullMove();

    void StartingPosition();

    // returns true after sucsessful execution, false otherwise
    bool InitialiseFromFen(std::array<std::string_view, 6> fen);
    bool InitialiseFromFen(std::string_view fen);

    bool is_repitition(int distance_from_root) const;
    bool is_two_fold_repitition() const;

    const BoardState& Board() const;
    const BoardState& PrevBoard() const;

private:
    BoardState& MutableBoard();

    std::vector<BoardState> previousStates;

    void update_current_position_repitition();
};
