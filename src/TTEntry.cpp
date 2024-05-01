#include "TTEntry.h"

#include <assert.h>
#include <climits>

#include "BitBoardDefine.h"

Score convert_to_tt_score(Score val, int distance_from_root)
{
    if (val >= Score::tb_win_in(MAX_DEPTH))
        return val - distance_from_root;
    if (val <= Score::tb_loss_in(MAX_DEPTH))
        return val + distance_from_root;
    return val;
}

Score convert_from_tt_score(Score val, int distance_from_root)
{
    if (val >= Score::tb_win_in(MAX_DEPTH))
        return val + distance_from_root;
    if (val <= Score::tb_loss_in(MAX_DEPTH))
        return val - distance_from_root;
    return val;
}

uint8_t get_generation(int currentTurnCount, int distanceFromRoot)
{
    return (currentTurnCount - distanceFromRoot) % (HALF_MOVE_MODULO) + 1;
}