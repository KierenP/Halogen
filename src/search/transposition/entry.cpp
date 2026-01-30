#include "search/transposition/entry.h"

#include "search/score.h"

namespace Transposition
{

Score convert_to_tt_score(Score val, int distance_from_root) noexcept
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val.is_win())
        return val + distance_from_root;
    if (val.is_loss())
        return val - distance_from_root;
    return val;
}

Score convert_from_tt_score(Score val, int distance_from_root) noexcept
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val.is_win())
        return val - distance_from_root;
    if (val.is_loss())
        return val + distance_from_root;
    return val;
}

int8_t get_generation(int currentTurnCount, int distanceFromRoot) noexcept
{
    return (currentTurnCount - distanceFromRoot) % GENERATION_MAX;
}

}