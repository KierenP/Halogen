#pragma once
#include "Move.h"
#include "MoveList.h"

class GameState;
struct SearchLocalState;
struct SearchStackState;

enum class Stage
{
    TT_MOVE,
    GEN_LOUD,
    GIVE_GOOD_LOUD,
    GEN_KILLER_1,
    GIVE_KILLER_1,
    GEN_KILLER_2,
    GIVE_KILLER_2,
    GIVE_BAD_LOUD,
    GEN_QUIET,
    GIVE_QUIET
};

// Encapsulation of staged move generation. To loop through moves:
//
// while (MoveGenerator.Next(move)) {
//     ...
// }
//
// Also responsible for updating history and killer tables

class StagedMoveGenerator
{
public:
    StagedMoveGenerator(const GameState& position, const SearchStackState* ss, SearchLocalState& local, Move tt_move,
        bool good_loud_only = false);

    // Returns false if no more legal moves
    bool Next(Move& move);

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void AdjustQuietHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;
    void AdjustLoudHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;

    // Signal the MoveGenerator that the LMP condition is satisfied and it should skip quiet moves
    void SkipQuiets();

    // Note this will be the stage of the coming move, not the one that was last returned.
    Stage GetStage() const
    {
        return stage;
    }

    Move TTMove()
    {
        return TTmove;
    }

private:
    void OrderQuietMoves(ExtendedMoveList& moves);
    void OrderLoudMoves(ExtendedMoveList& moves);

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    const bool good_loud_only;
    ExtendedMoveList loudMoves;
    ExtendedMoveList bad_loudMoves;
    ExtendedMoveList quietMoves;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator current;

    const Move TTmove = Move::Uninitialized;
    Move Killer1 = Move::Uninitialized;
    Move Killer2 = Move::Uninitialized;

    bool skipQuiets = false;
};
