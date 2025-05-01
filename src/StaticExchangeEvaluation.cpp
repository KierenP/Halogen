#include "StaticExchangeEvaluation.h"

#include <array>
#include <cstdint>
#include <iostream>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "Score.h"

uint64_t AttackersToSq(const BoardState& board, Square sq, uint64_t occ)
{
    uint64_t pawn_mask = (board.GetPieceBB<PAWN, WHITE>() & PawnAttacks[BLACK][sq]);
    pawn_mask |= (board.GetPieceBB<PAWN, BLACK>() & PawnAttacks[WHITE][sq]);

    uint64_t bishops = board.GetPieceBB<QUEEN>() | board.GetPieceBB<BISHOP>();
    uint64_t rooks = board.GetPieceBB<QUEEN>() | board.GetPieceBB<ROOK>();

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

    uint64_t attack_def = AttackersToSq(board, to, occ);

    const auto white_pinned = PinnedMask<WHITE>(board);
    const auto black_pinned = PinnedMask<BLACK>(board);
    const auto white_king_to_ray = RayBB[to][board.GetKing(WHITE)];
    const auto black_king_to_ray = RayBB[to][board.GetKing(BLACK)];
    const auto allowed = ~(white_pinned & ~white_king_to_ray) & ~(black_pinned & ~black_king_to_ray);

    attack_def &= allowed;
    scores[index] = PieceValues[captured];

    do
    {
        index++;
        attacker = !attacker;
        scores[index] = PieceValues[capturing] - scores[index - 1];

        attack_def ^= from_set;
        occ ^= from_set;

        attack_def |= occ & ((bishops & AttackBB<BISHOP>(to, occ)) | (rooks & AttackBB<ROOK>(to, occ)));
        attack_def &= allowed;
        from_set = GetLeastValuableAttacker(board, attack_def, capturing, Players(attacker));
    } while (from_set);
    while (--index)
    {
        scores[index - 1] = -(-scores[index - 1] > scores[index] ? -scores[index - 1] : scores[index]);
    }
    return scores[0];
}

bool see_ge(const BoardState& board, Move move, Score threshold)
{
    Square from = move.GetFrom();
    Square to = move.GetTo();

    auto capturing = board.GetSquare(from);
    auto attacker = ColourOfPiece(capturing);
    auto captured = move.GetFlag() == EN_PASSANT ? Piece(PAWN, !attacker) : board.GetSquare(to);

    // The value of 'swap' is the net exchanged material less the threshold, relative to the perspective to move. If the
    // value of the captured piece does not beat the threshold, the opponent can do nothing and we lose
    Score swap = PieceValues[captured] - threshold + 1;
    if (swap <= 0)
    {
        return false;
    }

    // From the opponents perspective, if recapturing does not beat the threshold we win
    swap = PieceValues[capturing] - swap + 1;
    if (swap <= 0)
    {
        return true;
    }

    auto stm = board.stm;
    int result = 1;
    uint64_t occ = board.GetAllPieces(), bishop_queen = 0, rook_queen = 0;

    bishop_queen = rook_queen = board.GetPieceBB<QUEEN>();
    bishop_queen |= board.GetPieceBB<BISHOP>();
    rook_queen |= board.GetPieceBB<ROOK>();

    if (move.GetFlag() == EN_PASSANT)
    {
        occ ^= SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];
    }

    occ ^= SquareBB[from];
    uint64_t attack_def = AttackersToSq(board, to, occ);
    attack_def ^= SquareBB[from];

    const auto white_pinned = PinnedMask<WHITE>(board);
    const auto black_pinned = PinnedMask<BLACK>(board);
    const auto white_king_to_ray = RayBB[to][board.GetKing(WHITE)];
    const auto black_king_to_ray = RayBB[to][board.GetKing(BLACK)];
    const auto allowed = ~(white_pinned & ~white_king_to_ray) & ~(black_pinned & ~black_king_to_ray);

    attack_def &= allowed;

    while (true)
    {
        stm = !stm;
        attack_def &= occ;
        auto stmAttackers = attack_def & board.GetPiecesColour(stm);

        // If the side to move has no more attackers, stm loses
        if (!stmAttackers)
        {
            break;
        }

        result ^= 1;

        // Find the least valuable attacker. Depending on the type of piece moved, we might also look for x-ray
        // attackers
        {
            auto pawns = stmAttackers & board.GetPieceBB(PAWN, stm);
            if (pawns)
            {
                swap = PieceValues[PAWN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(pawns)];
                attack_def |= occ & bishop_queen & AttackBB<BISHOP>(to, occ) & RayBB[to][LSB(pawns)];
                continue;
            }
        }

        {
            auto knights = stmAttackers & board.GetPieceBB(KNIGHT, stm);
            if (knights)
            {
                swap = PieceValues[KNIGHT] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(knights)];
                continue;
            }
        }

        {
            auto bishops = stmAttackers & board.GetPieceBB(BISHOP, stm);
            if (bishops)
            {
                swap = PieceValues[BISHOP] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(bishops)];
                attack_def |= occ & bishop_queen & AttackBB<BISHOP>(to, occ) & RayBB[to][LSB(bishops)];
                continue;
            }
        }

        {
            auto rooks = stmAttackers & board.GetPieceBB(ROOK, stm);
            if (rooks)
            {
                swap = PieceValues[ROOK] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(rooks)];
                attack_def |= occ & rook_queen & AttackBB<ROOK>(to, occ) & RayBB[to][LSB(rooks)];
                continue;
            }
        }

        {
            auto queens = stmAttackers & board.GetPieceBB(QUEEN, stm);
            if (queens)
            {
                swap = PieceValues[QUEEN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(queens)];
                attack_def |= occ
                    & ((bishop_queen & AttackBB<BISHOP>(to, occ)) | (rook_queen & AttackBB<ROOK>(to, occ)))
                    & RayBB[to][LSB(queens)];
                continue;
            }
        }

        {
            // if we only have the king available to attack we lose if the opponent has any further
            // attackers, and we would win on the next iteration if they don't
            if (attack_def & board.GetPiecesColour(!stm))
            {
                result ^= 1;
            }

            break;
        }
    }

    return result;
}
