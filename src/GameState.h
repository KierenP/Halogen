#pragma once

#include <string>
#include <vector>

#include "BoardState.h"
#include "Move.h"
#include "Network.h"
#include "Zobrist.h"

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

    bool CheckForRep(int distanceFromRoot, int maxReps) const;

    const BoardState& Board() const;
    const BoardState& PrevBoard() const;

private:
    BoardState& MutableBoard();

    std::vector<BoardState> previousStates;
};
