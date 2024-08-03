#pragma once

#include "MoveList.h"
#include "Network.h"

// Holds information about the search state for a particular recursion depth.
struct SearchStackState
{
    SearchStackState(int distance_from_root_);
    void reset();

    BasicMoveList pv = {};
    std::array<Move, 2> killers = {};

    Move move = Move::Uninitialized;
    Pieces moved_piece = N_PIECES;
    Move singular_exclusion = Move::Uninitialized;
    int multiple_extensions = 0;
    int nmp_verification_depth = 0;
    bool nmp_verification_root = false;
    const int distance_from_root;

    Accumulator acc;
};

class SearchStack
{
    // The search accesses [ss-1, ss+1]
    constexpr static int min_access = -1;
    constexpr static int max_access = 1;
    constexpr static size_t size = MAX_DEPTH + max_access - min_access;

public:
    const SearchStackState* root() const;
    SearchStackState* root();
    void reset();

private:
    std::array<SearchStackState, size> search_stack_array_ { generate(std::make_integer_sequence<int, size>()) };

    template <int... distances_from_root>
    decltype(search_stack_array_) generate(std::integer_sequence<int, distances_from_root...>)
    {
        return { (distances_from_root + min_access)... };
    }
};