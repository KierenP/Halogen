#include "StaticExchangeEvaluation.h"

#include "BoardState.h"
#include "MoveGeneration.h"
#include "Score.h"

BB AttackersToSq(const BoardState& board, Square sq, BB occ)
{
    BB pawn_mask = (board.GetPieceBB<PAWN, WHITE>() & PawnAttacks[BLACK][sq]);
    pawn_mask |= (board.GetPieceBB<PAWN, BLACK>() & PawnAttacks[WHITE][sq]);

    BB bishops = board.GetPieceBB<QUEEN>() | board.GetPieceBB<BISHOP>();
    BB rooks = board.GetPieceBB<QUEEN>() | board.GetPieceBB<ROOK>();

    return (pawn_mask & board.GetPieceBB<PAWN>()) | (AttackBB<KNIGHT>(sq) & board.GetPieceBB<KNIGHT>())
        | (AttackBB<KING>(sq) & board.GetPieceBB<KING>()) | (AttackBB<BISHOP>(sq, occ) & bishops)
        | (AttackBB<ROOK>(sq, occ) & rooks);
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
    BB occ = board.GetAllPieces(), bishop_queen = BB::none, rook_queen = BB::none;

    bishop_queen = rook_queen = board.GetPieceBB<QUEEN>();
    bishop_queen |= board.GetPieceBB<BISHOP>();
    rook_queen |= board.GetPieceBB<ROOK>();

    if (move.GetFlag() == EN_PASSANT)
    {
        occ ^= SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];
    }

    occ ^= SquareBB[from];
    BB attack_def = AttackersToSq(board, to, occ);
    attack_def ^= SquareBB[from];

    while (true)
    {
        stm = !stm;
        attack_def &= occ;
        auto stmAttackers = attack_def & board.GetPiecesColour(stm);

        // If the side to move has no more attackers, stm loses
        if (stmAttackers == BB::none)
        {
            break;
        }

        result ^= 1;

        // Find the least valuable attacker. Depending on the type of piece moved, we might also look for x-ray
        // attackers
        {
            auto pawns = stmAttackers & board.GetPieceBB(PAWN, stm);
            if (pawns != BB::none)
            {
                swap = PieceValues[PAWN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(pawns)];
                attack_def |= occ & (bishop_queen & AttackBB<BISHOP>(to, occ));
                continue;
            }
        }

        {
            auto knights = stmAttackers & board.GetPieceBB(KNIGHT, stm);
            if (knights != BB::none)
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
            if (bishops != BB::none)
            {
                swap = PieceValues[BISHOP] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(bishops)];
                attack_def |= occ & (bishop_queen & AttackBB<BISHOP>(to, occ));
                continue;
            }
        }

        {
            auto rooks = stmAttackers & board.GetPieceBB(ROOK, stm);
            if (rooks != BB::none)
            {
                swap = PieceValues[ROOK] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(rooks)];
                attack_def |= occ & (rook_queen & AttackBB<ROOK>(to, occ));
                continue;
            }
        }

        {
            auto queens = stmAttackers & board.GetPieceBB(QUEEN, stm);
            if (queens != BB::none)
            {
                swap = PieceValues[QUEEN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[LSB(queens)];
                attack_def
                    |= occ & ((bishop_queen & AttackBB<BISHOP>(to, occ)) | (rook_queen & AttackBB<ROOK>(to, occ)));
                continue;
            }
        }

        {
            // if we only have the king available to attack we lose if the opponent has any further
            // attackers, and we would win on the next iteration if they don't
            if ((attack_def & board.GetPiecesColour(!stm)) != BB::none)
            {
                result ^= 1;
            }

            break;
        }
    }

    return result;
}
