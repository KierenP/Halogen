#include "TTEntry.h"

#include <assert.h>
#include <climits>

#include "BitBoardDefine.h"

TTEntry::TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int currentTurnCount, int distanceFromRoot, EntryType Cutoff)
    : bestMove(best)
{
    assert(Score < SHRT_MAX && Score > SHRT_MIN);
    assert(Depth < CHAR_MAX && Depth > CHAR_MIN);

    key = ZobristKey;
    bestMove = best;
    score = static_cast<short>(Score);
    depth = static_cast<char>(Depth);
    cutoff = Cutoff;
    SetHalfMove(currentTurnCount, distanceFromRoot);
}

void TTEntry::MateScoreAdjustment(int distanceFromRoot)
{
    //checkmate node or TB win/loss
    if (score > EVAL_MAX)
        score -= static_cast<short>(distanceFromRoot);
    if (score < EVAL_MIN)
        score += static_cast<short>(distanceFromRoot);
}

void TTEntry::Reset()
{
    bestMove = Move::Uninitialized;
    key = EMPTY;
    score = -1;
    depth = -1;
    cutoff = EntryType::EMPTY_ENTRY;
    halfmove = -1;
}

void TTBucket::Reset()
{
    entry[0].Reset();
    entry[1].Reset();
    entry[2].Reset();
    entry[3].Reset();
}
