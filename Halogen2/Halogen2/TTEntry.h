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
	uint64_t key;			
	short int score;		
	signed char depth;		
	Move bestMove;			  
	bool ancient;			
	NodeCut cutoff;			 
};

