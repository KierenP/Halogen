#pragma once
#include <array>
#include <cstdint>
#include <type_traits>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Score.h"
#include "atomic.h"

Score convert_to_tt_score(Score val, int distance_from_root);
Score convert_from_tt_score(Score val, int distance_from_root);
uint8_t get_generation(int currentTurnCount, int distanceFromRoot);

constexpr static int GENERATION_MAX = 1 << 6;

struct TTMeta
{
    SearchResultType type : 2;
    uint8_t generation : 6;
};

// 16 bytes
class TTEntry
{
public:
    TTEntry() = default;

    atomic_relaxed<uint16_t> key = 0; // 2 bytes
    atomic_relaxed<Move> move = Move::Uninitialized; // 2 bytes
    atomic_relaxed<Score> score { 0 }; // 2 bytes
    atomic_relaxed<Score> static_eval { 0 }; // 2 bytes
    atomic_relaxed<int8_t> depth = 0; // 1 bytes
    atomic_relaxed<TTMeta> meta { TTMeta { SearchResultType::EMPTY, 0 } }; // 1 byte
};

struct alignas(32) TTBucket : public std::array<TTEntry, 3>
{
    constexpr static auto size = 3;
};

static_assert(sizeof(TTEntry) == 10);
static_assert(sizeof(TTBucket) == 32);
static_assert(alignof(TTBucket) == 32);
static_assert(std::is_trivially_copyable_v<TTBucket>);
static_assert(std::is_trivially_destructible_v<TTBucket>);
