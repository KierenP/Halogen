#pragma once
#include "Move.h"
#include "MoveList.h"

#include <coroutine>

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

class MoveGenerator
{
public:
    struct promise_type
    {
        Move move;

        auto get_return_object()
        {
            return MoveGenerator { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        void return_void() noexcept { }

        std::suspend_always yield_value(Move m) noexcept
        {
            move = m;
            return {};
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit MoveGenerator(handle_type h)
        : handle(h)
    {
    }

    ~MoveGenerator()
    {
        handle.destroy();
    }

    bool next()
    {
        handle.resume();
        return !handle.done();
    }

    Move get() const
    {
        return handle.promise().move;
    }

private:
    handle_type handle;
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

    MoveGenerator generate();

    // Signal the generator that a fail high has occured, and history tables need to be updated
    void AdjustQuietHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;
    void AdjustLoudHistory(const Move& move, int positive_adjustment, int negative_adjustment) const;

    // Signal the MoveGenerator that the LMP condition is satisfied and it should skip quiet moves
    void SkipQuiets();

    Move TTMove()
    {
        return TTmove;
    }

    bool AtBadLoudMoves()
    {
        return at_bad_loudMoves;
    }

private:
    void OrderQuietMoves(ExtendedMoveList& moves);
    void OrderLoudMoves(ExtendedMoveList& moves);

    // Data needed for use in ordering or generating moves
    const GameState& position;
    SearchLocalState& local;
    const SearchStackState* ss;
    bool quiescence;
    ExtendedMoveList loudMoves;
    ExtendedMoveList bad_loudMoves;
    ExtendedMoveList quietMoves;

    const Move TTmove = Move::Uninitialized;
    Move Killer1 = Move::Uninitialized;
    Move Killer2 = Move::Uninitialized;

    bool skipQuiets = false;
    bool at_bad_loudMoves = false;
};
