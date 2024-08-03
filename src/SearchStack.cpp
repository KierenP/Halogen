#include "SearchStack.h"

SearchStackState::SearchStackState(int distance_from_root_)
    : distance_from_root(distance_from_root_)
{
}

void SearchStackState::reset()
{
    pv = {};
    killers = {};
    move = Move::Uninitialized;
    singular_exclusion = Move::Uninitialized;
    multiple_extensions = 0;
    acc = {};
}

const SearchStackState* SearchStack::root() const
{
    return &search_stack_array_[-min_access];
}

SearchStackState* SearchStack::root()
{
    return &search_stack_array_[-min_access];
}

void SearchStack::reset()
{
    for (auto& sss : search_stack_array_)
    {
        sss.reset();
    }
}
