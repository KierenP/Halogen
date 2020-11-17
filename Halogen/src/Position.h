#pragma once

#include "BoardParamiters.h"
#include "BitBoard.h"
#include "Zobrist.h"
#include "Network.h"
#include <sstream>

#ifdef _MSC_VER 
#define NOMINMAX
#endif 

constexpr size_t NodeCountChunk = 1 << 12;
constexpr size_t NodeChunkMask = NodeCountChunk - 1;

/*
This class holds all the data required to define a chess board position, as well as some functions to manipulate and extract this data in convienient ways.
*/

class Position : public BoardParamiters, public BitBoard
{
public:
	Position();																									
	~Position();

	void ApplyMove(Move move);
	void ApplyMove(std::string strmove);
	void RevertMove();

	void ApplyNullMove();
	void RevertNullMove();

	void Print() const;

	void StartingPosition();
	bool InitialiseFromFen(std::vector<std::string> fen);
	bool InitialiseFromFen(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount); //Returns true after sucsessful execution, false otherwise
	bool InitialiseFromFen(std::string fen);

	uint64_t GetZobristKey() const;

	void Reset();

	size_t GetPreviousKeysSize() const { return PreviousKeys.size(); }
	uint64_t GetPreviousKey(size_t index);

	/*Seriously, don't use these functions outside of static exchange evaluation*/
	void ApplySEECapture(Move move);	//does ApplyMove functionality but much quicker. Only for use within see() and seeAttack()
	void RevertSEECapture();			//does RevertMove functionality but much quicker. Only for use within see() and seeAttack()

	Network* net;

	int16_t GetEvaluation();

	void addTbHit() { tbHits++; }
	bool NodesSearchedAddToThreadTotal() { return (nodesSearched & NodeChunkMask) == 0; }
	bool TbHitaddToThreadTotal() { return (tbHits & NodeChunkMask) == 0; }
	size_t GetNodes() { return nodesSearched; }

private:
	size_t nodesSearched;
	size_t tbHits;

	uint64_t key;
	std::vector<uint64_t> PreviousKeys;

	uint64_t GenerateZobristKey() const;
	uint64_t IncrementZobristKey(Move move);	

	std::array<int16_t, INPUT_NEURONS> GetInputLayer() const;
	deltaArray& CalculateMoveDelta(Move move);				//A vector which calculates the CHANGE in each input paramiter

	static size_t modifier(size_t index);								//no inputs for pawns on front or back rank for neural net: we need to modify zobrist-like indexes

	size_t EvaluatedPositions = 0;

	deltaArray delta;										//re recycle this object to save time in CalculateMoveDelta
};

