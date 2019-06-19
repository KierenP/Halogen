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

ABnode::ABnode(Move bestmove, int depth, unsigned int cutoff, int score, std::shared_ptr<ABnode> child)
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
}

int ABnode::GetScore() const
{
	return m_Score;
}

Move ABnode::GetMove() const
{
	return m_BestMove;
}

std::shared_ptr<ABnode> ABnode::GetChild() const
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

void ABnode::SetChild(std::shared_ptr<ABnode> child)
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
	//unsigned int depth = 1;
	//for (ABnode* ptr = this; ptr->HasChild(); ptr = ptr->GetChild())
		//depth++;
	if (HasChild())
		return m_Child->CountNodeChildren() + 1;
	return 1;	
}

void ABnode::PrintNodeChildren()
{
	GetMove().Print();
	std::cout << " ";

	if (!HasChild())
		return;

	m_Child->PrintNodeChildren();
}

std::shared_ptr<ABnode> CreateLeafNode(Position& position, int depth)
{
	return std::make_shared<ABnode>(Move(), depth, EXACT, EvaluatePosition(position));
}

std::shared_ptr<ABnode> CreateBranchNode(Move & move, int depth)
{
	return std::make_shared<ABnode>(move, depth, UNINITIALIZED_NODE, 0);
}

std::shared_ptr<ABnode> CreatePlaceHolderNode(bool colour, int depth)
{
	if (colour == WHITE)
		return std::make_shared<ABnode>(Move(), depth, UNINITIALIZED_NODE, LowINF);
	else
		return std::make_shared<ABnode>(Move(), depth, UNINITIALIZED_NODE, HighINF);
}

std::shared_ptr<ABnode> CreateDrawNode(Move move, int depth)
{
	return std::make_shared<ABnode>(move, depth, THREE_FOLD_REP, 0);
}

