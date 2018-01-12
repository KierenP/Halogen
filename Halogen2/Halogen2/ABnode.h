#pragma once
#include "Move.h"

extern int TotalNodeCount;

enum AlphaBetaCutoff
{
	ALPHA_CUTOFF,
	BETA_CUTOFF,
	EXACT,
};

enum AlphaBetaType
{
	ROOT,				//the top node, has no parent
	BRANCH,				//A middle node, has both parent and child
	LEAF,				//the bottom node, had no children
	NONE,
};

class ABnode
{
public:
	ABnode();
	//explicit ABnode(ABnode* parent, unsigned int type, Move bestmove, int score = 0);
	//explicit ABnode(unsigned int type, Move& move);
	explicit ABnode(unsigned int cutoff, Move bestmove, int depth, int score = 0);
	~ABnode();

	int GetScore();
	unsigned int GetType();
	Move GetMove();
	ABnode* GetChild();
	ABnode* GetParent();
	unsigned int GetCutoff();
	int GetDepth();

	void SetScore(int score);
	void SetType(unsigned int type);
	void SetMove(Move& move);
	void SetChild(ABnode* child);
	void SetParent(ABnode* parent);
	void SetDepth(int depth);
	void SetCutoff(unsigned int cutoff);

	bool HasChild();
	bool HasParent();

private:
	int m_Score;				
	int m_Depth;
	unsigned int m_Type;
	unsigned int m_Cutoff;
	Move m_BestMove;			
	ABnode* m_Child;
	ABnode* m_Parent;
};

