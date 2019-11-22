#pragma once
#include "ABnode.h"

class TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, NodeCut Cutoff);
	~TTEntry();

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; } 	
	char GetDepth() const { return depth; }
	bool IsAncient() const { return ancient; }
	NodeCut GetCutoff() const { return cutoff; }
	Move GetMove() const { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }

private:
	//fits within 16 bytes.
	uint64_t key;			//8 bytes
	short int score;		//2 bytes
	signed char depth;		//1 bytes
	Move bestMove;			//3 bytes  
	bool ancient;			//1 byte 
	NodeCut cutoff;			//1 byte  (?) 
};

