#pragma once

#include "movegen/move.h"
#include "search/score.h"

#include <array>
#include <cstdint>
#include <type_traits>

enum class SearchResultType : uint8_t;

namespace Transposition
{

constexpr static int GENERATION_MAX = 1 << 6;

constexpr Score convert_to_tt_score(Score val, int distance_from_root)
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val.is_win())
        return val + distance_from_root;
    if (val.is_loss())
        return val - distance_from_root;
    return val;
}

constexpr Score convert_from_tt_score(Score val, int distance_from_root)
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val.is_win())
        return val - distance_from_root;
    if (val.is_loss())
        return val + distance_from_root;
    return val;
}

constexpr int8_t get_generation(int currentTurnCount, int distanceFromRoot)
{
    return (currentTurnCount - distanceFromRoot) % GENERATION_MAX;
}

struct Meta
{
    SearchResultType type : 2;
    uint8_t generation : 6;
};

// 10 bytes
class Entry
{
public:
    Entry() = default;

    uint16_t key; // 2 bytes
    Move move; // 2 bytes
    Score score; // 2 bytes
    Score static_eval; // 2 bytes
    int8_t depth; // 1 bytes
    Meta meta; // 1 byte
};

struct alignas(32) Bucket : public std::array<Entry, 3>
{
    constexpr static auto size = 3;
};

static_assert(sizeof(Entry) == 10);
static_assert(sizeof(Bucket) == 32);
static_assert(alignof(Bucket) == 32);
static_assert(std::is_trivial_v<Bucket>);

}