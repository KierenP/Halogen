#pragma once

#include "Move.h"
#include "MoveGeneration.h"

// A fast software-based method for upcoming cycle detection in search trees
// M. N. J. van Kervinck
//
// https://web.archive.org/web/20180713113001/http://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf

namespace cuckoo
{

inline std::array<uint64_t, 8192> table;
inline std::array<Move, 8192> move_table;

inline uint16_t H1(uint64_t h)
{
    return (h >> 32) & 0x1fff;
};

inline uint16_t H2(uint64_t h)
{
    return (h >> 48) & 0x1fff;
};

void init();

}