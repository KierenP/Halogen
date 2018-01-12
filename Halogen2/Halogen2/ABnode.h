#pragma once
#include "Move.h"

extern int TotalNodeCount;

enum AlphaBetaCutoff
{
	ALPHA_CUTOFF,
	BETA_CUTOFF,
	EXACT,
	NULL_MOVE_PRUNE,
	FUTILITY_PRUNE,
	UNINITIALIZED_NODE,
	FORCED_MOVE
};

class ABnode
{
public:
	ABnode();
	ABnode(Move bestmove, int depth, unsigned int cutoff, int score, ABnode* child);
	~ABnode();

	int GetScore() const;
	int GetDepth() const;
	unsigned int GetCutoff() const;
	Move GetMove() const;
	ABnode* GetChild() const;

	void SetScore(int score);
	void SetDepth(int depth);
	void SetCutoff(unsigned int cutoff);
	void SetMove(Move& move);
	void SetChild(ABnode* child);

	bool HasChild() const;

	unsigned TraverseNodeChildren();														//interativly accesses sucsessive children and returns the true depth of the node

private:
	int m_Score;				
	int m_Depth;
	unsigned int m_Cutoff;
	Move m_BestMove;			
	ABnode* m_Child;
};

