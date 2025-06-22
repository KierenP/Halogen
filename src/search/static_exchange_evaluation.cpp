#include "search/static_exchange_evaluation.h"

#include <array>
#include <cstdint>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/score.h"

// movegen::attacks_to_sq could be used here?
uint64_t attackers_to_sq(const BoardState& board, Square sq, uint64_t occ)
{
    uint64_t pawn_mask = (board.get_pieces_bb<PAWN, WHITE>() & PawnAttacks[BLACK][sq]);
    pawn_mask |= (board.get_pieces_bb<PAWN, BLACK>() & PawnAttacks[WHITE][sq]);

    uint64_t bishops = board.get_pieces_bb<QUEEN>() | board.get_pieces_bb<BISHOP>();
    uint64_t rooks = board.get_pieces_bb<QUEEN>() | board.get_pieces_bb<ROOK>();

    return (pawn_mask & board.get_pieces_bb<PAWN>()) | (attack_bb<KNIGHT>(sq) & board.get_pieces_bb<KNIGHT>())
        | (attack_bb<KING>(sq) & board.get_pieces_bb<KING>()) | (attack_bb<BISHOP>(sq, occ) & bishops)
        | (attack_bb<ROOK>(sq, occ) & rooks);
}

uint64_t least_valuable_attacker(const BoardState& board, uint64_t attackers, Piece& capturing, Side side)
{
    for (int i = 0; i < 6; i++)
    {
        capturing = get_piece(PieceType(i), side);
        uint64_t pieces = board.get_pieces_bb(capturing) & attackers;
        if (pieces)
            return pieces & (~pieces + 1); // isolate LSB
    }
    return 0;
}

int see(const BoardState& board, Move move)
{
    Square from = move.from();
    Square to = move.to();

    int scores[32] { 0 };
    int index = 0;

    auto capturing = board.get_square_piece(from);
    auto attacker = enum_to<Side>(capturing);
    auto captured = move.flag() == EN_PASSANT ? get_piece(PAWN, !attacker) : board.get_square_piece(to);

    uint64_t from_set = (1ull << from);
    uint64_t occ = board.get_pieces_bb(), bishops = 0, rooks = 0;

    bishops = rooks = board.get_pieces_bb<QUEEN>();
    bishops |= board.get_pieces_bb<BISHOP>();
    rooks |= board.get_pieces_bb<ROOK>();

    if (move.flag() == EN_PASSANT)
    {
        occ ^= SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];
    }

    uint64_t attack_def = attackers_to_sq(board, to, occ);
    scores[index] = PieceValues[captured];

    do
    {
        index++;
        attacker = !attacker;
        scores[index] = PieceValues[capturing] - scores[index - 1];

        attack_def ^= from_set;
        occ ^= from_set;

        attack_def |= occ & ((bishops & attack_bb<BISHOP>(to, occ)) | (rooks & attack_bb<ROOK>(to, occ)));
        from_set = least_valuable_attacker(board, attack_def, capturing, Side(attacker));
    } while (from_set);
    while (--index)
    {
        scores[index - 1] = -(-scores[index - 1] > scores[index] ? -scores[index - 1] : scores[index]);
    }
    return scores[0];
}

bool see_ge(const BoardState& board, Move move, Score threshold)
{
    Square from = move.from();
    Square to = move.to();

    auto capturing = board.get_square_piece(from);
    auto attacker = enum_to<Side>(capturing);
    auto captured = move.flag() == EN_PASSANT ? get_piece(PAWN, !attacker) : board.get_square_piece(to);

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
    uint64_t occ = board.get_pieces_bb(), bishop_queen = 0, rook_queen = 0;

    bishop_queen = rook_queen = board.get_pieces_bb<QUEEN>();
    bishop_queen |= board.get_pieces_bb<BISHOP>();
    rook_queen |= board.get_pieces_bb<ROOK>();

    if (move.flag() == EN_PASSANT)
    {
        occ ^= SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];
    }

    occ ^= SquareBB[from];
    uint64_t attack_def = attackers_to_sq(board, to, occ);
    attack_def ^= SquareBB[from];

    while (true)
    {
        stm = !stm;
        attack_def &= occ;
        auto stmAttackers = attack_def & board.get_pieces_bb(stm);

        // If the side to move has no more attackers, stm loses
        if (!stmAttackers)
        {
            break;
        }

        result ^= 1;

        // Find the least valuable attacker. Depending on the type of piece moved, we might also look for x-ray
        // attackers
        {
            auto pawns = stmAttackers & board.get_pieces_bb(PAWN, stm);
            if (pawns)
            {
                swap = PieceValues[PAWN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[lsb(pawns)];
                attack_def |= occ & (bishop_queen & attack_bb<BISHOP>(to, occ));
                continue;
            }
        }

        {
            auto knights = stmAttackers & board.get_pieces_bb(KNIGHT, stm);
            if (knights)
            {
                swap = PieceValues[KNIGHT] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[lsb(knights)];
                continue;
            }
        }

        {
            auto bishops = stmAttackers & board.get_pieces_bb(BISHOP, stm);
            if (bishops)
            {
                swap = PieceValues[BISHOP] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[lsb(bishops)];
                attack_def |= occ & (bishop_queen & attack_bb<BISHOP>(to, occ));
                continue;
            }
        }

        {
            auto rooks = stmAttackers & board.get_pieces_bb(ROOK, stm);
            if (rooks)
            {
                swap = PieceValues[ROOK] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[lsb(rooks)];
                attack_def |= occ & (rook_queen & attack_bb<ROOK>(to, occ));
                continue;
            }
        }

        {
            auto queens = stmAttackers & board.get_pieces_bb(QUEEN, stm);
            if (queens)
            {
                swap = PieceValues[QUEEN] - swap + 1;
                if (swap <= 0)
                    break;
                occ ^= SquareBB[lsb(queens)];
                attack_def
                    |= occ & ((bishop_queen & attack_bb<BISHOP>(to, occ)) | (rook_queen & attack_bb<ROOK>(to, occ)));
                continue;
            }
        }

        {
            // if we only have the king available to attack we lose if the opponent has any further
            // attackers, and we would win on the next iteration if they don't
            if (attack_def & board.get_pieces_bb(!stm))
            {
                result ^= 1;
            }

            break;
        }
    }

    return result;
}
