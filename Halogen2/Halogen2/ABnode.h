#pragma once
#include "Move.h"
#include "Position.h"
#include "Evaluate.h"
#include <memory>

extern int TotalNodeCount;
class ABnode;

enum AlphaBetaCutoff
{
	ALPHA_CUTOFF,
	BETA_CUTOFF,
	EXACT,
	NULL_MOVE_PRUNE,
	FUTILITY_PRUNE,
	UNINITIALIZED_NODE,
	FORCED_MOVE,
	CHECK_MATE,				//note checkmate is also an 'exact' evaluation, that is, we did not do a cutoff and made a blind assumption
	THREE_FOLD_REP
};

enum Scores
{
	HighINF = 30000,
	LowINF = -30000,

	WhiteLoses = -9999,
	BlackLoses = 9999,

	Draw = 0
};

ABnode* CreateLeafNode(Position& position, int depth);			//returns the pointer to a terminal leaf node
ABnode* CreateBranchNode(Move& move, int depth);				//returns the pointer to a branch node who's score and cutoff are still to be set by its children
ABnode* CreatePlaceHolderNode(bool colour, int depth);			//Pass either HighINF or LowINF to set the 'best' node to this in initialization
ABnode* CreateForcedNode(Move& move);							//In the event of searching a position AT THE ROOT LEVEL and only one legal move being available, we can create a node with a cutoff of EXACT and a given move
ABnode* CreateCheckmateNode(bool colour, int depth);	
ABnode* CreateDrawNode(Move move, int depth);

class ABnode
{
public:
	ABnode();
	ABnode(Move bestmove, int depth, unsigned int cutoff, int score, ABnode* child = nullptr);
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

	unsigned CountNodeChildren();							//interativly accesses sucsessive children and returns the true depth of the node
	void PrintNodeChildren();

private:
	int m_Score;				
	int m_Depth;
	unsigned int m_Cutoff;
	Move m_BestMove;			
	ABnode* m_Child;
};

