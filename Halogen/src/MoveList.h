#pragma once
#include "Move.h"
#include <array>

// normally we would just generate a vector of Move, but later
// we need to convert that vector to a vector of ExtendedMove. 
// This is expensive, and its actually better to just create
// the ExtendedMove vector from the start.
struct ExtendedMove
{
	ExtendedMove() = default;
	ExtendedMove(const Move _move, const int _score = 0, const short int _SEE = 0) : move(_move), score(_score), SEE(_SEE) {}
	ExtendedMove(Square from, Square to, MoveFlag flag) : move(from, to, flag), score(0), SEE(0) {}

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
public:
	MoveList() = default;
	
	using iterator = std::array<ExtendedMove, 256>::iterator;

	void Append(ExtendedMove move);

	//perfect forwarding might be faster? like vector::emplace_back
	template <typename... Args>
	void Append(Args&&... args);

	iterator begin();
	iterator end();
	size_t size() const;
	void clear();
	void erase(size_t index);

	const ExtendedMove& operator[](size_t index) const { return list[index]; }
	      ExtendedMove& operator[](size_t index)       { return list[index]; }

private:
	std::array<ExtendedMove, 256> list;
	size_t moveCount = 0;
};

template <typename ...Args>
inline void MoveList::Append(Args&& ...args)
{
	list[moveCount] = ExtendedMove(std::forward<Args>(args)...);
	moveCount++;
}
