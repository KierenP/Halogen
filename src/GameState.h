#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "BitBoardDefine.h"
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
    void ApplyMove(const std::string& strmove);
    void RevertMove();

    void ApplyNullMove();
    void RevertNullMove();

    void StartingPosition();

    // returns true after sucsessful execution, false otherwise
    // TODO: are all these needed?
    bool InitialiseFromFen(std::vector<std::string> fen);
    bool InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle,
        const std::string& ep, const std::string& fiftyMove, const std::string& turnCount);
    bool InitialiseFromFen(std::string fen);

    // TODO: is this needed?
    void Reset();

    Score GetEvaluation() const;

    bool CheckForRep(int distanceFromRoot, int maxReps) const;

    Move GetPreviousMove() const;

    const BoardState& Board() const;

private:
    BoardState& MutableBoard();

    Network net;
    std::vector<Move> moveStack;

    std::vector<BoardState> previousStates;
};
