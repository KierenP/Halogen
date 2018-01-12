#pragma once
#include "PerftTT.h"
#include "ABnode.h"
#include "Move.h"

class TTEntry
{
public:
	TTEntry(uint64_t ZobristKey = EMPTY, int Score = -1, int Depth = -1, unsigned int Cutoff = NONE);
	TTEntry(Move best, uint64_t ZobristKey = EMPTY, int Score = -1, int Depth = -1, unsigned int Cutoff = NONE);
	~TTEntry();

	uint64_t GetKey() { return key; }
	int GetScore() { return score; }		
	char GetDepth() { return depth; }
	bool IsAncient() { return ancient; }
	char GetCutoff() { return cutoff; }
	Move GetMove() { return bestMove; }

	void SetAncient(bool isAncient) { ancient = isAncient; }

private:
	//21 bytes 
	uint64_t key;			//8 bytes
	int score;				//4 bytes
	int depth;				//4 bytes
	Move bestMove;			//3 bytes  will be brought down to 1 soon
	bool ancient;			//1 byte 
	char cutoff;			//1 byte   
};

