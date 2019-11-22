#pragma once
#include "Move.h"
#include "Position.h"
#include "Evaluate.h"

class ABnode;

enum class NodeCut
{
	ALPHA_CUTOFF,
	BETA_CUTOFF,
	EXACT_CUTOFF,
	UNINITIALIZED_NODE,
	CHECK_MATE_CUTOFF,				//note checkmate is also an 'exact' evaluation, that is, we did not do a cutoff which made a blind assumption
	THREE_FOLD_REP_CUTOFF,
	QUIESSENCE_NODE_CUTOFF,
	FUTILE_NODE,
};

enum class Score
{
	HighINF = 30000,
	LowINF = -30000,

	WhiteWin = 9999,
	BlackWin = -9999,

	Draw = 0
};

ABnode* CreateLeafNode(Position& position, int depth);			//returns the pointer to a terminal leaf node
ABnode* CreateBranchNode(Move& move, int depth);				//returns the pointer to a branch node who's score and cutoff are still to be set by its children
ABnode* CreatePlaceHolderNode(bool colour, int depth);			//Pass either HighINF or LowINF to set the 'best' node to this in initialization

class ABnode
{
public:
	ABnode(Move bestmove, int depth, NodeCut cutoff, int score, ABnode* child = nullptr);
	~ABnode();

	int GetScore() const;
	int GetDepth() const;
	NodeCut GetCutoff() const;
	Move GetMove() const;
	ABnode* GetChild() const;

	void SetScore(int score);
	void SetCutoff(NodeCut cutoff);
	void SetChild(ABnode* child);							//NOTE: will delete the current child if it has one!

	bool HasChild() const;

	unsigned CountNodeChildren();							//interativly accesses sucsessive children and returns the true depth of the node
	void PrintNodeChildren();

private:
	int m_Score;				
	int m_Depth;											//note this is depth remaining!
	NodeCut m_Cutoff;
	Move m_BestMove;			
	ABnode* m_Child;
};

