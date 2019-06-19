#pragma once
#include "Move.h"
#include "Position.h"
#include "Evaluate.h"
#include <memory>

class ABnode;

enum AlphaBetaCutoff
{
	ALPHA_CUTOFF,
	BETA_CUTOFF,
	EXACT,
	UNINITIALIZED_NODE,
	CHECK_MATE,				//note checkmate is also an 'exact' evaluation, that is, we did not do a cutoff which made a blind assumption
	THREE_FOLD_REP,
	QUIESSENCE_NODE,
};

enum ScoreConstant
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
	int m_Depth;											//note this is depth remaining!
	unsigned int m_Cutoff;
	Move m_BestMove;			
	ABnode* m_Child;
};

