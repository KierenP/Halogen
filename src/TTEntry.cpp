#include "TTEntry.h"

#include <assert.h>
#include <climits>

#include "BitBoardDefine.h"

TTEntry::TTEntry(Move best, uint64_t ZobristKey, Score score, int depth, int currentTurnCount, int distanceFromRoot,
    EntryType cutoff)
    : key_(ZobristKey)
    , bestMove_(best)
    , score_(score)
    , depth_(depth)
    , cutoff_(cutoff)
{
    assert(depth < CHAR_MAX && depth > CHAR_MIN);

    SetHalfMove(currentTurnCount, distanceFromRoot);
}

void TTEntry::MateScoreAdjustment(int distanceFromRoot)
{
    // checkmate node or TB win/loss
    // TODO: check boundary conditions
    if (score_ > Score::Limits::EVAL_MAX)
        score_ -= static_cast<short>(distanceFromRoot);
    if (score_ < Score::Limits::EVAL_MIN)
        score_ += static_cast<short>(distanceFromRoot);
}

void TTEntry::Reset()
{
    bestMove_ = Move::Uninitialized;
    key_ = EMPTY;
    score_ = -1;
    depth_ = -1;
    cutoff_ = EntryType::EMPTY_ENTRY;
    halfmove_ = -1;
}

void TTBucket::Reset()
{
    entry[0].Reset();
    entry[1].Reset();
    entry[2].Reset();
    entry[3].Reset();
}
