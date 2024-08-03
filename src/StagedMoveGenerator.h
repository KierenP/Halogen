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
    GIVE_LOUD,
    GEN_QUIET,
    GIVE_QUIET,
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
    StagedMoveGenerator(const GameState& position, const SearchStackState* ss, SearchLocalState& local, Move tt_move);

    // Returns false if no more legal moves
    bool Next(Move& move);

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void AdjustQuietHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;

private:
    void OrderQuietMoves();

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    ExtendedMoveList loud_moves;
    ExtendedMoveList quiet_moves;
    const Move tt_move;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator current;
};
