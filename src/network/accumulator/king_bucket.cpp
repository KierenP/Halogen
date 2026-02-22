#include "king_bucket.h"

namespace NN::KingBucket
{

void KingBucketAccumulator::apply_lazy_updates(
    const KingBucketAccumulator& prev_acc, AccumulatorTable& table, const NN::network& net)
{
    if (white_requires_recalculation)
    {
        // King changed bucket for white — recalculate from table (sets kb + psq for WHITE)
        table.recalculate(*this, board, WHITE, board.get_king_sq(WHITE), net);

        // Incrementally update BLACK (kb + psq fused)
        if (n_adds == 1 && n_subs == 1)
            add1sub1(prev_acc, adds[0], subs[0], BLACK, net);
        else if (n_adds == 1 && n_subs == 2)
            add1sub2(prev_acc, adds[0], subs[0], subs[1], BLACK, net);
        else if (n_adds == 2 && n_subs == 2)
            add2sub2(prev_acc, adds[0], adds[1], subs[0], subs[1], BLACK, net);
    }
    else if (black_requires_recalculation)
    {
        // King changed bucket for black — recalculate from table (sets kb + psq for BLACK)
        table.recalculate(*this, board, BLACK, board.get_king_sq(BLACK), net);

        // Incrementally update WHITE (kb + psq fused)
        if (n_adds == 1 && n_subs == 1)
            add1sub1(prev_acc, adds[0], subs[0], WHITE, net);
        else if (n_adds == 1 && n_subs == 2)
            add1sub2(prev_acc, adds[0], subs[0], subs[1], WHITE, net);
        else if (n_adds == 2 && n_subs == 2)
            add2sub2(prev_acc, adds[0], adds[1], subs[0], subs[1], WHITE, net);
    }
    else
    {
        if (n_adds == 1 && n_subs == 1)
        {
            add1sub1(prev_acc, adds[0], subs[0], net);
        }
        else if (n_adds == 1 && n_subs == 2)
        {
            add1sub2(prev_acc, adds[0], subs[0], subs[1], net);
        }
        else if (n_adds == 2 && n_subs == 2)
        {
            add2sub2(prev_acc, adds[0], adds[1], subs[0], subs[1], net);
        }
        else if (n_adds == 0 && n_subs == 0)
        {
            // TODO: remove this branch, the search no longer applies incremental updates for null moves
            side = prev_acc.side;
        }
    }

    acc_is_valid = true;
}
}