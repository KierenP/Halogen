#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Score.h"

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

    std::atomic<uint16_t> key = 0; // 2 bytes
    std::atomic<Move> move = Move::Uninitialized; // 2 bytes
    std::atomic<Score> score { 0 }; // 2 bytes
    std::atomic<Score> static_eval { 0 }; // 2 bytes
    std::atomic<int8_t> depth = 0; // 1 bytes
    std::atomic<TTMeta> meta { TTMeta { SearchResultType::EMPTY, 0 } }; // 1 byte

    TTMeta get_meta()
    {
        return meta.load(std::memory_order_relaxed);
    }

    char unused[6];
};

struct alignas(64) TTBucket : public std::array<TTEntry, 4>
{
    constexpr static auto size = 4;
};

static_assert(sizeof(TTEntry) == 16, "TTEntry is not 16 bytes");
static_assert(sizeof(TTBucket) == 64, "TTBucket is not 64 bytes");
static_assert(alignof(TTBucket) == 64, "TTBucket alignment is not 64 bytes");
static_assert(std::is_trivially_copyable_v<TTBucket>);
static_assert(std::is_trivially_destructible_v<TTBucket>);
