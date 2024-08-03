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
    GEN_MOVES,
    GIVE_MOVES
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

private:
    void OrderMoves(ExtendedMoveList& moves);

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    ExtendedMoveList moves;
    const Move tt_move;

    // Data uses for keeping track of internal values
    Stage stage;
    ExtendedMoveList::iterator current;
};
