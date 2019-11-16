#pragma once
#include "BoardParamiters.h"
#include "BitBoard.h"
#include "Zobrist.h"
#include <sstream>
#include <Windows.h>

extern std::vector<uint64_t> PreviousKeys;
extern int NodeCount;

/*
This class holds all the data required to define a chess board position, as well as some functions to manipulate and extract this data in convienient ways.
*/

class Position : public BoardParamiters, public BitBoard
{
public:
	Position();
	Position(std::vector<std::string> moves);																								//Initialise from a vector of moves from the starting position
	Position(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount);		//split fen
	Position(std::string fen);																												//whole fen
	~Position();

	void ApplyMove(Move move);
	void ApplyMove(std::string strmove);
	void RevertMove(Move move);

	void ApplyNullMove();
	void RevertNullMove();

	void Print() const;

	void StartingPosition();
	bool InitialiseFromFen(std::vector<std::string> fen);
	bool InitialiseFromFen(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount); //Returns true after sucsessful execution, false otherwise
	bool InitialiseFromFen(std::string fen);
	bool InitialiseFromMoves(std::vector<std::string> moves);

	uint64_t GetZobristKey() const;

private:
	uint64_t key;
	uint64_t GenerateZobristKey() const;
};

