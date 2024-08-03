#include "StagedMoveGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

#include "BitBoardDefine.h"
#include "GameState.h"
#include "History.h"
#include "MoveGeneration.h"
#include "SearchData.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

StagedMoveGenerator::StagedMoveGenerator(
    const GameState& position_, const SearchStackState* ss_, SearchLocalState& local_, Move tt_move)
    : position(position_)
    , local(local_)
    , ss(ss_)
    , tt_move(tt_move)
    , stage(Stage::TT_MOVE)
{
}

bool StagedMoveGenerator::Next(Move& move)
{
    if (stage == Stage::TT_MOVE)
    {
        stage = Stage::GEN_LOUD;

        if (MoveIsLegal(position.Board(), tt_move))
        {
            move = tt_move;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        stage = Stage::GIVE_LOUD;
        QuiescenceMoves(position.Board(), loud_moves);
        OrderLoudMoves();
        current = loud_moves.begin();
    }

    if (stage == Stage::GIVE_LOUD)
    {
        if (current == loud_moves.end())
        {
            stage = Stage::GEN_QUIET;
        }
        else
        {
            move = current->move;
            current++;
            return true;
        }
    }

    if (stage == Stage::GEN_QUIET)
    {
        stage = Stage::GIVE_QUIET;
        QuietMoves(position.Board(), quiet_moves);
        OrderQuietMoves();
        current = quiet_moves.begin();
    }

    if (stage == Stage::GIVE_QUIET)
    {
        if (current == quiet_moves.end())
        {
            return false;
        }
        else
        {
            move = current->move;
            current++;
            return true;
        }
    }

    assert(false);
    __builtin_unreachable();
}

void StagedMoveGenerator::AdjustQuietHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.quiet_history.add(position, ss, move, positive_adjustment);

    for (auto const& m : quiet_moves)
    {
        if (m.move == move)
            break;

        local.quiet_history.add(position, ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::AdjustLoudHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.loud_history.add(position, ss, move, positive_adjustment);

    for (auto const& m : loud_moves)
    {
        if (m.move == move)
            break;

        local.loud_history.add(position, ss, m.move, negative_adjustment);
    }
}

void selection_sort(ExtendedMoveList& v)
{
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        std::iter_swap(it, std::max_element(it, v.end()));
    }
}

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000, 91, 532, 568, 715, 1279, 5000 };

uint64_t AttackersToSq(const BoardState& board, Square sq)
{
    uint64_t pawn_mask = (board.GetPieceBB<PAWN, WHITE>() & PawnAttacks[BLACK][sq]);
    pawn_mask |= (board.GetPieceBB<PAWN, BLACK>() & PawnAttacks[WHITE][sq]);

    uint64_t bishops = board.GetPieceBB<QUEEN>() | board.GetPieceBB<BISHOP>();
    uint64_t rooks = board.GetPieceBB<QUEEN>() | board.GetPieceBB<ROOK>();
    uint64_t occ = board.GetAllPieces();

    return (pawn_mask & board.GetPieceBB<PAWN>()) | (AttackBB<KNIGHT>(sq) & board.GetPieceBB<KNIGHT>())
        | (AttackBB<KING>(sq) & board.GetPieceBB<KING>()) | (AttackBB<BISHOP>(sq, occ) & bishops)
        | (AttackBB<ROOK>(sq, occ) & rooks);
}

uint64_t GetLeastValuableAttacker(const BoardState& board, uint64_t attackers, Pieces& capturing, Players side)
{
    for (int i = 0; i < 6; i++)
    {
        capturing = Piece(PieceTypes(i), side);
        uint64_t pieces = board.GetPieceBB(capturing) & attackers;
        if (pieces)
            return pieces & (~pieces + 1); // isolate LSB
    }
    return 0;
}

int see(const BoardState& board, Move move)
{
    Square from = move.GetFrom();
    Square to = move.GetTo();

    int scores[32] { 0 };
    int index = 0;

    auto capturing = board.GetSquare(from);
    auto attacker = ColourOfPiece(capturing);
    auto captured = move.GetFlag() == EN_PASSANT ? Piece(PAWN, !attacker) : board.GetSquare(to);

    uint64_t from_set = (1ull << from);
    uint64_t occ = board.GetAllPieces(), bishops = 0, rooks = 0;

    bishops = rooks = board.GetPieceBB<QUEEN>();
    bishops |= board.GetPieceBB<BISHOP>();
    rooks |= board.GetPieceBB<ROOK>();

    if (move.GetFlag() == EN_PASSANT)
    {
        occ ^= SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];
    }

    uint64_t attack_def = AttackersToSq(board, to);
    scores[index] = PieceValues[captured];

    do
    {
        index++;
        attacker = !attacker;
        scores[index] = PieceValues[capturing] - scores[index - 1];

        if (-scores[index - 1] < 0 && scores[index] < 0)
            break;

        attack_def ^= from_set;
        occ ^= from_set;

        attack_def |= occ & ((bishops & AttackBB<BISHOP>(to, occ)) | (rooks & AttackBB<ROOK>(to, occ)));
        from_set = GetLeastValuableAttacker(board, attack_def, capturing, Players(attacker));
    } while (from_set);
    while (--index)
    {
        scores[index - 1] = -(-scores[index - 1] > scores[index] ? -scores[index - 1] : scores[index]);
    }
    return scores[0];
}

void StagedMoveGenerator::OrderQuietMoves()
{
    auto swap_erase = [this](size_t i)
    {
        std::iter_swap(quiet_moves.begin() + i, quiet_moves.end() - 1);
        quiet_moves.pop_back();
    };

    for (size_t i = 0; i < quiet_moves.size(); i++)
    {
        // Hash move
        if (quiet_moves[i].move == tt_move)
        {
            swap_erase(i);
            i--;
        }
        else
        {
            quiet_moves[i].score = local.quiet_history.get(position, ss, quiet_moves[i].move);
        }
    }

    selection_sort(quiet_moves);
}

void StagedMoveGenerator::OrderLoudMoves()
{
    auto swap_erase = [this](size_t i)
    {
        std::iter_swap(loud_moves.begin() + i, loud_moves.end() - 1);
        loud_moves.pop_back();
    };

    for (size_t i = 0; i < loud_moves.size(); i++)
    {
        // Hash move
        if (loud_moves[i].move == tt_move)
        {
            swap_erase(i);
            i--;
        }
        else
        {
            loud_moves[i].score = local.loud_history.get(position, ss, loud_moves[i].move);
        }
    }

    selection_sort(loud_moves);
}