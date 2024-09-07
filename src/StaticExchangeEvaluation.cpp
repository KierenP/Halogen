#include "StaticExchangeEvaluation.h"

#include "BoardState.h"
#include "MoveGeneration.h"

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