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

unsigned ABnode::TraverseNodeChildren()
{
	unsigned int depth = 1;
	for (ABnode* ptr = this; ptr->HasChild(); ptr = ptr->GetChild())
		depth++;
	return depth;
}




