#pragma once
#include "ABnode.h"

enum class EntryType {
	EMPTY,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

class TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, EntryType Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	char GetDepth() const { return depth; }
	bool IsAncient() const { return ancient; }
	EntryType GetCutoff() const { return cutoff; }
	Move GetMove() const { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }
	void MateScoreAdjustment(int distanceFromRoot);

private:
	//fits within 16 bytes.
	uint64_t key;			//8 bytes
	short int score;		//2 bytes
	signed char depth;		//1 bytes
	Move bestMove;			//3 bytes  
	bool ancient;			//1 byte 
	EntryType cutoff;		//1 byte  (?) 
};

