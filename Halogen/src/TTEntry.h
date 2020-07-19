#pragma once
#include "Move.h"
#include "BitBoardDefine.h"

enum class EntryType {
	EMPTY_ENTRY,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

//16 bytes
class TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int halfmove, int distanceFromRoot, EntryType Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	int GetDepth() const { return depth; }
	bool IsAncient(int currenthalfmove, int distanceFromRoot) const { return halfmove != (currenthalfmove - distanceFromRoot) % 256; }
	EntryType GetCutoff() const { return static_cast<EntryType>(cutoff); }
	Move GetMove() const { return bestMove; }

	void SetHalfMove(int currenthalfmove, int distanceFromRoot) { halfmove = (currenthalfmove - distanceFromRoot) % 256; }	//halfmove is from current position, distanceFromRoot adjusts this to get what the halfmove was at the root of the search
	void MateScoreAdjustment(int distanceFromRoot);

	void Reset();

private:
	/*Arranged to minimize padding*/

	uint64_t key;			//8 bytes

	Move bestMove;			//2 bytes 
	char halfmove;			//1 bytes		(is stored as the halfmove at the ROOT of this current search, modulo 256)

	short int score;		//2 bytes
	char depth;				//1 bytes
	char cutoff;			//1 bytes
};

