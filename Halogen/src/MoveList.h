#pragma once
#include "Move.h"
#include <array>
#include <functional>

// For move ordering we need to bundle the 'score' and SEE values
// with the move objects
struct ExtendedMove
{
	ExtendedMove() = default;
	ExtendedMove(const Move _move, const int _score = 0, const short int _SEE = 0) : move(_move), score(_score), SEE(_SEE) {}
	ExtendedMove(Square from, Square to, MoveFlag flag) : move(from, to, flag) {}

	bool operator<(const ExtendedMove& rhs) const { return score < rhs.score; };
	bool operator>(const ExtendedMove& rhs) const { return score > rhs.score; };

	Move move;
	int16_t score = 0;
	int16_t SEE = 0;
};

// Internally, a MoveList is an array
// You can Append() moves to the list and it will internally keep track of how many moves have been added
// You can get iterators at the Begin() and End() of the move list
class MoveList
{
private:
	std::array<ExtendedMove, 256> list;

public:
	//Pass arguments to construct the ExtendedMove()
	template <typename... Args>
	void Append(Args&&... args);

	using iterator = decltype(list)::iterator;

	iterator begin();
	iterator end();

	size_t size() const;

	void clear();
	void erase(size_t index);

	const ExtendedMove& operator[](size_t index) const { return list[index]; }
	      ExtendedMove& operator[](size_t index)       { return list[index]; }

private:
	size_t moveCount = 0;
};

template <typename ...Args>
inline void MoveList::Append(Args&& ...args)
{
	list[moveCount++] = { args... };
}

// Assumption: We want to use MoveList objects in a LIFO fashion, always throwing away
// the one that was most recently allocated. Because the search algorithm is recursive, 
// this is a reasonable assumption.

// Assumption: We won't ever call Get() to get more than MAX_DEPTH * 2 'MoveList's. This is 
// justified as Get() is only called on each recursion of NegaScout or Quiessence and we
// should never have more than MAX_DEPTH * 2 recursions. We double MAX_DEPTH because 
// the assumption is not a strong one and in cases like a null move verification search, we 
// may recurse without incrementing distanceFromRoot allowing more than MAX_DEPTH recursions

// MoveListStack acts as an object pool of MoveList objects. It is ~300KB in size. It is
// reasonable to assume this will allow the use of enough MoveList objects simultaneously
// without running out of space.

// Get() returns a MoveListPtr (unique_ptr). When the unique_ptr is destroyed, it signals
// the MoveListPool that this MoveList can now be resused automatically. Take care to manage
// ownership of the unique_ptr to last as long as you need to use the MoveList and no longer.

class MoveListPool
{
private:
	std::array<MoveList, MAX_DEPTH * 2> lists;
	size_t listCount = 0;

	void Free(MoveList*) { listCount--; }

public:
	using MoveListPtr = std::unique_ptr<MoveList, std::function<void(MoveList*)>>;
	auto Get() { return MoveListPtr(&lists[listCount++], [this](MoveList*) { Free(nullptr); }); }
};