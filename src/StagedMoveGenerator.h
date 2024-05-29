#pragma once
#include <cstdint>
#include <optional>

#include "Move.h"
#include "MoveList.h"
#include "SearchData.h"

class GameState;

enum class Stage
{
    TT_MOVE,
    GEN_LOUD,
    GIVE_GOOD_LOUD,
    GEN_KILLER_1,
    GIVE_KILLER_1,
    GEN_KILLER_2,
    GIVE_KILLER_2,
    GEN_QUIET_CHECKS,
    GIVE_QUIET_CHECKS,
    GIVE_BAD_LOUD,
    GEN_QUIET_NON_CHECKS,
    GIVE_QUIET_NON_CHECKS
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
    StagedMoveGenerator(
        const GameState& position, const SearchStackState* ss, SearchLocalState& local, Move tt_move, bool Quiescence);

    // Returns false if no more legal moves
    bool Next(Move& move);

    // Get the static exchange evaluation of the given move
    // Optimized to return the SEE without calculation if it
    // was already calculated and used in move ordering
    int16_t GetSEE(Move move) const;

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void AdjustHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;

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
    void OrderMoves(ExtendedMoveList& moves);

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    const bool quiescence;
    ExtendedMoveList loudMoves;
    ExtendedMoveList quiet_checks;
    ExtendedMoveList quiet_non_checks;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator loud_current;
    ExtendedMoveList::iterator quiet_checks_current;
    ExtendedMoveList::iterator quiet_non_checks_current;

    // We use SEE for ordering the moves, but SEE is also used in QS.
    // See the body of GetSEE for usage.
    std::optional<int16_t> moveSEE;

    const Move TTmove = Move::Uninitialized;
    Move Killer1 = Move::Uninitialized;
    Move Killer2 = Move::Uninitialized;

    bool skipQuiets = false;
};
