#include "search/cuckoo.h"

#include "bitboard/define.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/zobrist.h"

// A fast software-based method for upcoming cycle detection in search trees
// M. N. J. van Kervinck
//
// https://web.archive.org/web/20180713113001/http://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf

namespace Cuckoo
{

bool is_valid_and_reversible_move(Move move, Piece piece)
{
    const auto from = move.from();
    const auto to = move.to();

    switch (enum_to<PieceType>(piece))
    {
    case KNIGHT:
        return (attack_bb<KNIGHT>(from) & SquareBB[to]);
    case BISHOP:
        return (attack_bb<BISHOP>(from) & SquareBB[to]);
    case ROOK:
        return (attack_bb<ROOK>(from) & SquareBB[to]);
    case QUEEN:
        return (attack_bb<QUEEN>(from) & SquareBB[to]);
    case KING:
        return (attack_bb<KING>(from) & SquareBB[to]);
    default:
        return false;
    }
}

void insert(uint64_t move_hash, Move move)
{
    auto index = H1(move_hash);

    while (true)
    {
        std::swap(table[index], move_hash);
        std::swap(move_table[index], move);
        if (move_hash == 0)
        {
            // Arrived at empty slot so we’re done for this move
            break;
        }
        // Push victim to its alternate slot
        index = (index == H1(move_hash)) ? H2(move_hash) : H1(move_hash);
    }
}

void init()
{
    // loop through all valid and reversible moves. The only reversible moves are QUIET non-pawn moves. We can half the
    // table size by considering from-to and to-from as the same move. It is known that we expect exactly 3668 moves.

    [[maybe_unused]] int count = 0;

    for (int i = 0; i < N_PIECES; i++)
    {
        const auto piece = Piece(i);
        for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        {
            for (Square sq2 = Square(sq1 + 1); sq2 <= SQ_H8; ++sq2)
            {
                const auto move = Move(sq1, sq2, QUIET);

                if (!is_valid_and_reversible_move(move, piece))
                {
                    continue;
                }

                const uint64_t move_hash = Zobrist::piece_square(piece, move.from())
                    ^ Zobrist::piece_square(piece, move.to()) ^ Zobrist::stm();
                insert(move_hash, move);
                count++;
            }
        }
    }

    assert(count == 3668);
}

}