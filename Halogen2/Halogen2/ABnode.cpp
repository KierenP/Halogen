#include "ABnode.h"

int TotalNodeCount = 0;

ABnode::ABnode()
{
	m_Cutoff = UNINITIALIZED_NODE;
	m_Depth = -1;
	m_Score = -1;
	m_Child = nullptr;
	m_BestMove = Move();
}

ABnode::ABnode(Move bestmove, int depth, unsigned int cutoff, int score, ABnode* child)
{
	m_Cutoff = cutoff;
	m_Depth = depth;
	m_Score = score;
	m_Child = child;
	m_BestMove = bestmove;
}

ABnode::~ABnode()
{
	TotalNodeCount++;
	
	if (HasChild())
		delete m_Child;
}

int ABnode::GetScore() const
{
	return m_Score;
}

Move ABnode::GetMove() const
{
	return m_BestMove;
}

ABnode * ABnode::GetChild() const
{
	return m_Child;
}

unsigned int ABnode::GetCutoff() const
{
	return m_Cutoff;
}

int ABnode::GetDepth() const
{
	return m_Depth;
}

void ABnode::SetScore(int score)
{
	m_Score = score;
}

void ABnode::SetMove(Move & move)
{
	m_BestMove = move;
}

void ABnode::SetChild(ABnode * child)
{
	m_Child = child;
	m_Score = child->GetScore();
	m_Cutoff = child->GetCutoff();
}

void ABnode::SetDepth(int depth)
{
	m_Depth = depth;
}

void ABnode::SetCutoff(unsigned int cutoff)
{
	m_Cutoff = cutoff;
}

bool ABnode::HasChild() const
{
	if (m_Child != nullptr)
		return true;
	return false;
}

unsigned ABnode::CountNodeChildren()
{
	unsigned int depth = 1;
	for (ABnode* ptr = this; ptr->HasChild(); ptr = ptr->GetChild())
		depth++;
	return depth - 1;	//Currently at the end of the line is a leaf node with no move, so we -1 so account for that
}

void ABnode::PrintNodeChildren()
{
	for (ABnode* ptr = this; ptr->HasChild(); ptr = ptr->GetChild())
	{
		std::cout << " ";
		ptr->GetMove().Print();
	}
}

ABnode* CreateLeafNode(Position& position, int depth)
{
	return new ABnode(Move(), depth, EXACT, EvaluatePosition(position));
}

ABnode * CreateBranchNode(Move & move, int depth)
{
	return new ABnode(move, depth, UNINITIALIZED_NODE, 0);
}

ABnode * CreatePlaceHolderNode(bool colour, int depth)
{
	if (colour == WHITE)
		return new ABnode(Move(), depth, UNINITIALIZED_NODE, LowINF);
	else
		return new ABnode(Move(), depth, UNINITIALIZED_NODE, HighINF);
}

ABnode * CreateForcedNode(Move & move)
{
	return new ABnode(move, 0, FORCED_MOVE, 0);
}

ABnode * CreateCheckmateNode(bool colour, int depth)
{
	//more depths still to search produce more desirable results for the winning side.

	if (colour == WHITE)
		return new ABnode(Move(), depth, CHECK_MATE, WhiteLoses - depth);
	if (colour == BLACK)
		return new ABnode(Move(), depth, CHECK_MATE, BlackLoses + depth);
}

ABnode * CreateDrawNode(Move& move, int depth)
{
	return new ABnode(move, depth, THREE_FOLD_REP, 0);
}
