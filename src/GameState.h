#pragma once

#include <string>
#include <vector>

#include "BoardState.h"
#include "Move.h"
#include "Network.h"
#include "Zobrist.h"

// Information required to undo a move
struct BoardStateInfo
{
    Zobrist key;
    Square en_passant;
    uint64_t castle_squares;
    unsigned int fifty_move_count;

    // unset in the case of en passant
    Pieces captured_piece;
};

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
    void RevertMove(Move move);

    void ApplyNullMove();
    void RevertNullMove();

    void StartingPosition();

    // returns true after sucsessful execution, false otherwise
    bool InitialiseFromFen(std::array<std::string_view, 6> fen);
    bool InitialiseFromFen(std::string_view fen);

    Score GetEvaluation() const;

    bool CheckForRep(int distanceFromRoot, int maxReps) const;

    const BoardState& Board() const;

private:
    Network net;
    BoardState board;

    std::vector<BoardStateInfo> state_stack;
};
