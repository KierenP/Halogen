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

// 16 bytes
class TTEntry
{
public:
    TTEntry() = default;
    TTEntry(Move best, uint64_t ZobristKey, Score score, int Depth, int currentTurnCount, int distanceFromRoot,
        EntryType Cutoff);

    bool IsAncient(unsigned int currentTurnCount, unsigned int distanceFromRoot) const
    {
        return halfmove_ != CalculateAge(currentTurnCount, distanceFromRoot);
    }

    __attribute__((no_sanitize("thread"))) void SetHalfMove(int currentTurnCount, int distanceFromRoot)
    {
        // halfmove is from current position, distanceFromRoot adjusts this to get what the halfmove was at the root of
        // the search
        halfmove_ = CalculateAge(currentTurnCount, distanceFromRoot);
    }

    void MateScoreAdjustment(int distanceFromRoot);
    void Reset();

    static uint8_t CalculateAge(int currentTurnCount, int distanceFromRoot)
    {
        return (currentTurnCount - distanceFromRoot) % (HALF_MOVE_MODULO) + 1;
    }

    uint64_t GetKey() const
    {
        return key_;
    }

    Score GetScore() const
    {
        return score_;
    }

    int GetDepth() const
    {
        return depth_;
    }

    EntryType GetCutoff() const
    {
        return cutoff_;
    }

    char GetAge() const
    {
        return halfmove_;
    }

    Move GetMove() const
    {
        return bestMove_;
    }

private:
    /*Arranged to minimize padding*/
    uint64_t key_ = 0; // 8 bytes
    Move bestMove_ = Move::Uninitialized; // 2 bytes
    Score score_ = 0; // 2 bytes
    int8_t depth_ = 0; // 1 bytes
    EntryType cutoff_ = EntryType::EMPTY_ENTRY; // 1 bytes
    // is stored as the move count at the ROOT of this current search modulo 16 plus 1
    int8_t halfmove_ = 0; // 1 bytes
};

struct TTBucket
{
    constexpr static size_t SIZE = 4;

    void Reset();
    alignas(64) std::array<TTEntry, SIZE> entry;
};

static_assert(sizeof(TTEntry) == 16, "TTEntry is not 16 bytes");
static_assert(sizeof(TTBucket) == 64, "TTBucket is not 64 bytes");
static_assert(std::is_trivially_copyable_v<TTBucket>);
