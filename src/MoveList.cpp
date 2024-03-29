#include "MoveList.h"

template <typename T>
typename FixedVector<T>::iterator FixedVector<T>::begin()
{
    return list.begin();
}

template <typename T>
typename FixedVector<T>::iterator FixedVector<T>::end()
{
    return list.begin() + moveCount;
}

template <typename T>
typename FixedVector<T>::const_iterator FixedVector<T>::begin() const
{
    return list.begin();
}

template <typename T>
typename FixedVector<T>::const_iterator FixedVector<T>::end() const
{
    return list.begin() + moveCount;
}

template <typename T>
size_t FixedVector<T>::size() const
{
    return moveCount;
}

template <typename T>
void FixedVector<T>::clear()
{
    moveCount = 0;
}

template <typename T>
void FixedVector<T>::erase(size_t index)
{
    std::move(list.begin() + index + 1, list.end(), list.begin() + index);
    moveCount--;
}

// Explicit template instantiation
template class FixedVector<ExtendedMove>;
template class FixedVector<Move>;
