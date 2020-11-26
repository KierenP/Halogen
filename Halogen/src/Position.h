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
	void ApplyMoveQuick(Move move);	//does ApplyMove functionality but much quicker.
	void RevertMoveQuick();			//does RevertMove functionality but much quicker.

	int16_t GetEvaluation();

	void addTbHit() { tbHits++; }
	void addNode() { nodesSearched++; }
	bool NodesSearchedAddToThreadTotal();
	bool TbHitaddToThreadTotal();

	size_t GetNodes() { return nodesSearched; }

	bool CheckForRep(int distanceFromRoot, int maxReps);

	void ResetSeldepth() { selDepth = 0; }
	void ReportDepth(int distanceFromRoot) { selDepth = std::max(distanceFromRoot, selDepth); }
	int GetSelDepth() const { return selDepth; }

private:
	size_t nodesSearched;
	size_t tbHits;
	int selDepth;

	uint64_t key;
	std::vector<uint64_t> PreviousKeys;

	uint64_t GenerateZobristKey() const;
	uint64_t IncrementZobristKey(Move move);	

	std::array<int16_t, INPUT_NEURONS> GetInputLayer() const;
	deltaArray& CalculateMoveDelta(Move move);				//A vector which calculates the CHANGE in each input paramiter

	static size_t modifier(size_t index);								//no inputs for pawns on front or back rank for neural net: we need to modify zobrist-like indexes

	size_t EvaluatedPositions = 0;

	deltaArray delta;										//re recycle this object to save time in CalculateMoveDelta

	//Values for keeping the network updated
	std::array<std::array<int16_t, HIDDEN_NEURONS>, MAX_DEPTH + 1> Zeta;
	size_t incrementalDepth = 0;
};

