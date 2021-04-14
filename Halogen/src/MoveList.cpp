#include "MoveList.h"

void MoveList::Append(ExtendedMove move)
{
    list[moveCount] = move;
    moveCount++;
}

MoveList::iterator MoveList::begin()
{
    return list.begin();
}

MoveList::iterator MoveList::end()
{
    return list.begin() + moveCount;
}

size_t MoveList::size() const
{
    return moveCount;
}

void MoveList::clear()
{
    moveCount = 0;
}

void MoveList::erase(size_t index)
{
    std::move(list.begin() + index + 1, list.end(), list.begin() + index);
    moveCount--;
}
