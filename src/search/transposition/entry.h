#pragma once

#include "movegen/move.h"
#include "search/score.h"

#include <array>
#include <cstdint>
#include <type_traits>

enum class SearchResultType : uint8_t;

namespace Transposition
{

Score convert_to_tt_score(Score val, int distance_from_root);
Score convert_from_tt_score(Score val, int distance_from_root);
int8_t get_generation(int currentTurnCount, int distanceFromRoot);

constexpr static int GENERATION_MAX = 1 << 6;

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

struct alignas(64) Bucket : public std::array<Entry, 6>
{
    constexpr static auto size = 6;
};

static_assert(sizeof(Entry) == 10);
static_assert(sizeof(Bucket) == 64);
static_assert(alignof(Bucket) == 64);
static_assert(std::is_trivial_v<Bucket>);

}