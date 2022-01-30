#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "BitBoard.h"
#include "BitBoardDefine.h"
#include "BoardParameters.h"
#include "Move.h"
#include "Network.h"
#include "Zobrist.h"

constexpr size_t NodeCountChunk = 1 << 12;

/*
This class holds all the data required to define a chess board position, as well as some functions to manipulate and extract this data in convienient ways.
*/

class Position : public BoardParameters, public BitBoard
{
public:
    Position();

    void ApplyMove(Move move);
    void ApplyMove(const std::string& strmove);
    void RevertMove();

    void ApplyNullMove();
    void RevertNullMove();

    void Print() const;

    void StartingPosition();
    bool InitialiseFromFen(std::vector<std::string> fen);
    //Returns true after sucsessful execution, false otherwise
    bool InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle, const std::string& ep, const std::string& fiftyMove, const std::string& turnCount);
    bool InitialiseFromFen(std::string fen);

    uint64_t GetZobristKey() const;

    void Reset();

    /*Seriously, don't use these functions outside of static exchange evaluation*/
    void ApplyMoveQuick(Move move); //does ApplyMove functionality but much quicker.
    void RevertMoveQuick(); //does RevertMove functionality but much quicker.

    int16_t GetEvaluation() const;

    bool CheckForRep(int distanceFromRoot, int maxReps) const;

    Move GetPreviousMove() const;

private:
    Zobrist zobrist;
    std::vector<Zobrist> PreviousKeys;

    void SetSquareAndUpdate(Square square, Pieces piece);
    void ClearSquareAndUpdate(Square square);

    Network net;
    std::vector<Move> moveStack;
};
