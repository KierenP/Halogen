#include "ABnode.h"

//static int totalnodecount = 0;
int TotalNodeCount = 0;

ABnode::ABnode()
{
	m_Type = NONE;
	m_Cutoff = EXACT;
	m_Depth = -1;
	m_Score = -1;
}

ABnode::ABnode(unsigned int cutoff, Move bestmove, int depth, int score)
{
	m_Type = NONE;
	m_Cutoff = cutoff;
	m_Depth = depth;
	m_BestMove = bestmove;
	m_Score = score;
}

ABnode::~ABnode()
{
	TotalNodeCount++;
	
	if (HasChild())
		delete m_Child;
}

int ABnode::GetScore()
{
	return m_Score;
}

unsigned int ABnode::GetType()
{
	return m_Type;
}

Move ABnode::GetMove()
{
	return m_BestMove;
}

ABnode * ABnode::GetChild()
{
	return m_Child;
}

ABnode * ABnode::GetParent()
{
	return m_Parent;
}

unsigned int ABnode::GetCutoff()
{
	return m_Cutoff;
}

int ABnode::GetDepth()
{
	return m_Depth;
}



void ABnode::SetScore(int score)
{
	m_Score = score;
}

void ABnode::SetType(unsigned int type)
{
	m_Type = type;
}

void ABnode::SetMove(Move & move)
{
	m_BestMove = move;
}

void ABnode::SetChild(ABnode * child)
{
	m_Child = child;

	if (m_Type == NONE)
		m_Type = ROOT;
	if (m_Type == LEAF)
		m_Type = BRANCH;
}

void ABnode::SetParent(ABnode * parent)
{
	m_Parent = parent;

	if (m_Type == NONE)
		m_Type = LEAF;
	if (m_Type == ROOT)
		m_Type = BRANCH;
}

void ABnode::SetDepth(int depth)
{
	m_Depth = depth;
}

void ABnode::SetCutoff(unsigned int cutoff)
{
	m_Cutoff = cutoff;
}

bool ABnode::HasChild()
{
	/*if (m_Child != nullptr)
		return true;
	return false;*/

	if (m_Type == ROOT || m_Type == BRANCH)
		return true;
	return false;
}

bool ABnode::HasParent()
{
	/*if (m_Parent != nullptr)
		return true;
	return false;*/

	if (m_Type == LEAF || m_Type == BRANCH)
		return true;
	return false;
}




