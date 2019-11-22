#include "ABnode.h"

ABnode::ABnode(Move bestmove, int depth, NodeCut cutoff, int score, ABnode* child) : m_BestMove(bestmove)
{
	m_Cutoff = cutoff;
	m_Depth = depth;
	m_Score = score;
	m_Child = child;
}

ABnode::~ABnode()
{
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

NodeCut ABnode::GetCutoff() const
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

void ABnode::SetChild(ABnode * child)
{
	if (HasChild()) delete m_Child;	//avoiding memory leaks

	m_Child = child;
	m_Score = child->GetScore();
	m_Cutoff = child->GetCutoff();
}

void ABnode::SetCutoff(NodeCut cutoff)
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

	return depth;
}

void ABnode::PrintNodeChildren()
{
	GetMove().Print();

	if (!HasChild())
		return;

	ABnode* ptr = this;

	do
	{
		std::cout << " ";
		ptr = ptr->GetChild();
		ptr->GetMove().Print();
	} while (ptr->HasChild());
}

ABnode* CreateLeafNode(Position& position, int depth)
{
	return new ABnode(Move(), depth, NodeCut::EXACT_CUTOFF, EvaluatePosition(position));
}

ABnode * CreateBranchNode(Move & move, int depth)
{
	return new ABnode(move, depth, NodeCut::UNINITIALIZED_NODE, 0);
}

ABnode * CreatePlaceHolderNode(bool colour, int depth)
{
	if (colour == Players::WHITE)
		return new ABnode(Move(), depth, NodeCut::UNINITIALIZED_NODE, static_cast<int>(Score::LowINF));
	else
		return new ABnode(Move(), depth, NodeCut::UNINITIALIZED_NODE, static_cast<int>(Score::HighINF));
}
