#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "Move.h"

constexpr unsigned int HALF_MOVE_MODULO = 16;
constexpr size_t BucketSize = 4;

enum class EntryType : char
{
    EMPTY_ENTRY,
    EXACT,
    LOWERBOUND,
    UPPERBOUND
};

//16 bytes
class TTEntry
{
public:
    TTEntry() = default;
    TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int currentTurnCount, int distanceFromRoot, EntryType Cutoff);

    bool IsAncient(unsigned int currentTurnCount, unsigned int distanceFromRoot) const { return halfmove != static_cast<char>((currentTurnCount - distanceFromRoot) % (HALF_MOVE_MODULO)); }

    void SetHalfMove(int currentTurnCount, int distanceFromRoot) { halfmove = CalculateAge(currentTurnCount, distanceFromRoot); } //halfmove is from current position, distanceFromRoot adjusts this to get what the halfmove was at the root of the search
    void MateScoreAdjustment(int distanceFromRoot);
    void Reset();

    static uint8_t CalculateAge(int currenthalfmove, int distanceFromRoot) { return (currenthalfmove - distanceFromRoot) % (HALF_MOVE_MODULO); }

    uint64_t GetKey() const { return key; }
    int GetScore() const { return score; }
    int GetDepth() const { return depth; }
    EntryType GetCutoff() const { return cutoff; }
    char GetAge() const { return halfmove; }
    Move GetMove() const { return bestMove; }

private:
    /*Arranged to minimize padding*/
    uint64_t key; //8 bytes
    Move bestMove; //2 bytes
    short int score; //2 bytes
    char depth; //1 bytes
    EntryType cutoff; //1 bytes
    char halfmove; //1 bytes		(is stored as the halfmove at the ROOT of this current search, modulo 16)
};

struct TTBucket
{
    void Reset();
    std::array<TTEntry, BucketSize> entry;
};

static_assert(std::is_trivial_v<TTBucket>);
