#pragma once

#include "movegen/list.h"
#include "movegen/move.h"
#include "search/score.h"
#include "utility/fraction.h"

#include <cstdint>

class GameState;
struct SearchLocalState;
struct SearchStackState;

enum class Stage : int8_t
{
    PROBCUT_TT_MOVE,
    PROBCUT_GEN_LOUD,
    PROBCUT_GIVE_GOOD_LOUD,

    TT_MOVE,
    GEN_LOUD,
    GIVE_GOOD_LOUD,
    GEN_QUIET,
    GIVE_QUIET,
    GIVE_BAD_LOUD,
};

// Encapsulation of staged move generation. To loop through moves:
//
// while (MoveGenerator.Next(move)) {
//     ...
// }
//
// Also responsible for updating history

class StagedMoveGenerator
{
public:
    StagedMoveGenerator(const GameState& position, const SearchStackState* ss, SearchLocalState& local, Move tt_move,
        bool good_loud_only = false);

    static StagedMoveGenerator probcut(const GameState& position, const SearchStackState* ss, SearchLocalState& local,
        Move tt_move, Score probcut_see_margin);

    // Returns false if no more legal moves
    bool next(Move& move);

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void update_quiet_history(
        const Move& move, Fraction<64> positive_adjustment, Fraction<64> negative_adjustment) const;
    void update_loud_history(
        const Move& move, Fraction<64> positive_adjustment, Fraction<64> negative_adjustment) const;

    // Signal the MoveGenerator that the LMP condition is satisfied and it should skip quiet moves
    void skip_quiets();

    // Note this will be the stage of the coming move, not the one that was last returned.
    Stage get_stage() const
    {
        return stage;
    }

    Move tt_move()
    {
        return TTmove;
    }

private:
    void score_quiet_moves(BasicMoveList& moves);
    void score_loud_moves(BasicMoveList& moves);

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    bool good_loud_only;
    ExtendedMoveList loudMoves;
    ExtendedMoveList bad_loudMoves;
    ExtendedMoveList quietMoves;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator current;
    ExtendedMoveList::iterator sorted_end;

    const Move TTmove = Move::Uninitialized;

    bool skipQuiets = false;
    Score probcut_see_margin;
};
