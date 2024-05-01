#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "Move.h"
#include "Score.h"

constexpr unsigned int HALF_MOVE_MODULO = 16;

enum class EntryType : char
{
    EMPTY_ENTRY,
    EXACT,
    LOWERBOUND,
    UPPERBOUND
};

Score convert_to_tt_score(Score val, int distance_from_root);
Score convert_from_tt_score(Score val, int distance_from_root);
uint8_t get_generation(int currentTurnCount, int distanceFromRoot);

// 16 bytes
class TTEntry
{
public:
    TTEntry() = default;

    /*Arranged to minimize padding*/
    uint64_t key = 0; // 8 bytes
    Move move = Move::Uninitialized; // 2 bytes
    Score score = 0; // 2 bytes
    int8_t depth = 0; // 1 bytes
    EntryType cutoff = EntryType::EMPTY_ENTRY; // 1 bytes
    // is stored as the move count at the ROOT of this current search modulo 16 plus 1
    int8_t generation = 0; // 1 bytes
};

struct alignas(64) TTBucket : public std::array<TTEntry, 4>
{
    constexpr static auto size = 4;
};

static_assert(sizeof(TTEntry) == 16, "TTEntry is not 16 bytes");
static_assert(sizeof(TTBucket) == 64, "TTBucket is not 64 bytes");
static_assert(alignof(TTBucket) == 64, "TTBucket alignment is not 64 bytes");
static_assert(std::is_trivially_copyable_v<TTBucket>);
