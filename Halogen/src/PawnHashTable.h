#pragma once
#include "Position.h"

uint64_t PawnHashKey(const Position& pos, char gameStage);

struct PawnHash
{
	PawnHash(uint64_t hashKey = 0, int score = -1, bool used = false, char GameStage = N_STAGES) { key = hashKey; eval = score; occupied = used; gameStage = GameStage; }
	~PawnHash() {};

	uint64_t key;
	int eval;
	char gameStage;
	bool occupied;	
};

class PawnHashTable
{
public:
	PawnHashTable();
	~PawnHashTable();

	void ResetTable();
	void Init(unsigned int size);	//in MB!!

	bool CheckEntry(uint64_t key);
	PawnHash GetEntry(uint64_t key);
	void AddEntry(uint64_t hashKey, int score);

	uint64_t HashHits = 0;
	uint64_t HashMisses = 0;

private:
	std::vector<PawnHash> table;
};

