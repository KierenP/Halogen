#pragma once
#include <cstdint>
#include <optional>

#include "Move.h"
#include "MoveList.h"

class Position;
struct SearchData;

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
    StagedMoveGenerator(Position& Position, int DistanceFromRoot, const SearchData& Locals, bool Quiescence);

    // Returns false if no more legal moves
    bool Next(Move& move);

    // Get the static exchange evaluation of the given move
    // Optimized to return the SEE without calculation if it
    // was already calculated and used in move ordering
    int16_t GetSEE(Move move) const;

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void AdjustHistory(const Move& move, SearchData& Locals, int depthRemaining) const;

    // Signal the MoveGenerator that the LMP condition is satisfied and it should skip quiet moves
    void SkipQuiets();

    // Note this will be the stage of the coming move, not the one that was last returned.
    Stage GetStage() const { return stage; }

private:
    void OrderMoves(ExtendedMoveList& moves);

    // Data needed for use in ordering or generating moves
    Position& position;
    int distanceFromRoot;
    const SearchData& locals;
    bool quiescence;
    ExtendedMoveList loudMoves;
    ExtendedMoveList quietMoves;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator current;

    // We use SEE for ordering the moves, but SEE is also used in QS.
    // See the body of GetSEE for usage.
    std::optional<int16_t> moveSEE;

    Move TTmove = Move::Uninitialized;
    Move Killer1 = Move::Uninitialized;
    Move Killer2 = Move::Uninitialized;

    bool skipQuiets = false;
};

Move GetHashMove(const Position& position, int depthRemaining, int distanceFromRoot);
Move GetHashMove(const Position& position, int distanceFromRoot);