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
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, EntryType Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	int GetDepth() const { return depth; }
	bool IsAncient() const { return ancient; }
	EntryType GetCutoff() const { return static_cast<EntryType>(cutoff); }
	Move GetMove() const { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }
	void MateScoreAdjustment(int distanceFromRoot);

private:
	/*Arranged to minimize padding*/

	uint64_t key;			//8 bytes

	Move bestMove;			//2 bytes 
	bool ancient;			//1 bytes

	short int score;		//2 bytes
	char depth;				//1 bytes
	char cutoff;			//1 bytes
};

