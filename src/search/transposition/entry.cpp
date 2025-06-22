#include "search/transposition/entry.h"

#include "Score.h"
#include "bitboard.h"

namespace Transposition
{

Score convert_to_tt_score(Score val, int distance_from_root)
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val >= Score::tb_win_in(MAX_DEPTH))
        return val + distance_from_root;
    if (val <= Score::tb_loss_in(MAX_DEPTH))
        return val - distance_from_root;
    return val;
}

Score convert_from_tt_score(Score val, int distance_from_root)
{
    if (val == SCORE_UNDEFINED)
        return val;
    if (val >= Score::tb_win_in(MAX_DEPTH))
        return val - distance_from_root;
    if (val <= Score::tb_loss_in(MAX_DEPTH))
        return val + distance_from_root;
    return val;
}

int8_t get_generation(int currentTurnCount, int distanceFromRoot)
{
    return (currentTurnCount - distanceFromRoot) % GENERATION_MAX;
}

}