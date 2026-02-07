#pragma once

#include "movegen/move.h"

#include <array>
#include <cstdint>

// A fast software-based method for upcoming cycle detection in search trees
// M. N. J. van Kervinck
//
// https://web.archive.org/web/20180713113001/http://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf

namespace Cuckoo
{

inline std::array<uint64_t, 8192> table;
inline std::array<Move, 8192> move_table;

constexpr uint16_t H1(uint64_t h)
{
    return (h >> 32) & 0x1fff;
};

constexpr uint16_t H2(uint64_t h)
{
    return (h >> 48) & 0x1fff;
};

void init();

}