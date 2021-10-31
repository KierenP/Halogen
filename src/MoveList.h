#pragma once
#include "Move.h"
#include <array>
#include <functional>
#include <memory>

// For move ordering we need to bundle the 'score' and SEE values
// with the move objects
struct ExtendedMove
{
    ExtendedMove() = default;
    ExtendedMove(const Move _move)
        : move(_move)
        , score(0)
        , SEE(0)
    {
    }
    ExtendedMove(Square from, Square to, MoveFlag flag)
        : move(from, to, flag)
        , score(0)
        , SEE(0)
    {
    }

    //If you need a constructor that sets score or SEE then feel free to add one.

    bool operator<(const ExtendedMove& rhs) const { return score < rhs.score; };
    bool operator>(const ExtendedMove& rhs) const { return score > rhs.score; };

    Move move;
    int16_t score;
    int16_t SEE;
};

template <typename T>
class FixedVector;

using ExtendedMoveList = FixedVector<ExtendedMove>;
using BasicMoveList = FixedVector<Move>;

// Internally, a FixedVector is an array
// You can Append() moves to the list and it will internally keep track of how many moves have been added
// You can get iterators at the Begin() and End() of the move list

template <typename T>
class FixedVector
{
private:
    std::array<T, 256> list;
    static_assert(std::is_trivial_v<T>);

public:
    using iterator = typename decltype(list)::iterator;
    using const_iterator = typename decltype(list)::const_iterator;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    size_t size() const;

    void clear();
    void erase(size_t index);

    const T& operator[](size_t index) const { return list[index]; }
    T& operator[](size_t index) { return list[index]; }

    template <typename... Args>
    void emplace_back(Args&&... args)
    {
        list[moveCount++] = { args... };
    }

    template <class InputIt>
    void append(InputIt first, InputIt last)
    {
        std::copy(first, last, end());
        moveCount += std::distance(first, last);
    }

private:
    size_t moveCount = 0;
};
