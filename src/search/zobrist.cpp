#include "search/zobrist.h"

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "utility/splitmix64.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace Zobrist
{

constexpr std::array<uint64_t, 12 * 64 + 1 + 16 + 8> Table = []
{
    SplitMix64 rng(0);
    std::array<uint64_t, 12 * 64 + 1 + 16 + 8> table {};

    for (size_t i = 0; i < table.size(); i++)
    {
        table[i] = rng.next();
    }

    return table;
}();

uint64_t piece_square(Piece piece, Square square)
{
    return Table[piece * 64 + square];
}

uint64_t stm()
{
    return Table[12 * 64];
}

uint64_t castle(Square square)
{
    assert(enum_to<Rank>(square) == RANK_1 || enum_to<Rank>(square) == RANK_8);

    if (enum_to<Rank>(square) == RANK_1)
        return Table[12 * 64 + 1 + enum_to<File>(square)];
    else
        return Table[12 * 64 + 9 + enum_to<File>(square)];
}

uint64_t en_passant(File file)
{
    return Table[12 * 64 + 17 + file];
}

uint64_t key(const BoardState& board)
{
    uint64_t key = 0;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bitboard = board.get_pieces_bb(static_cast<Piece>(i));
        while (bitboard != 0)
        {
            key ^= piece_square(static_cast<Piece>(i), lsbpop(bitboard));
        }
    }

    if (board.stm == WHITE)
    {
        key ^= stm();
    }

    auto castle_rights = board.castle_squares;
    while (castle_rights != 0)
    {
        key ^= castle(lsbpop(castle_rights));
    }

    if (board.en_passant <= SQ_H8)
    {
        key ^= en_passant(enum_to<File>(board.en_passant));
    }

    return key;
}

uint64_t pawn_key(const BoardState& board)
{
    uint64_t key = 0;

    uint64_t white = board.get_pieces_bb(WHITE_PAWN);
    while (white != 0)
    {
        key ^= piece_square(WHITE_PAWN, lsbpop(white));
    }

    uint64_t black = board.get_pieces_bb(BLACK_PAWN);
    while (black != 0)
    {
        key ^= piece_square(BLACK_PAWN, lsbpop(black));
    }

    return key;
}

uint64_t non_pawn_key(const BoardState& board, Side side)
{
    uint64_t key = 0;

    for (int i = KNIGHT; i <= KING; i++)
    {
        const auto piece = get_piece(static_cast<PieceType>(i), side);
        uint64_t bitboard = board.get_pieces_bb(piece);
        while (bitboard != 0)
        {
            key ^= piece_square(piece, lsbpop(bitboard));
        }
    }

    return key;
}

constexpr size_t fifty_move_buckets = 10;

constexpr std::array<uint64_t, fifty_move_buckets> fifty_move_bucket_hash = []()
{
    // Continue from where the main Table left off (seed at same state)
    SplitMix64 rng(0);
    // Advance rng to the state after Table initialization
    for (size_t i = 0; i < 12 * 64 + 1 + 16 + 8; i++)
    {
        rng.next();
    }

    std::array<uint64_t, fifty_move_buckets> table {};
    for (size_t i = 0; i < fifty_move_buckets; i++)
    {
        table[i] = rng.next();
    }
    return table;
}();

constexpr size_t calculate_fifty_move_bucket(int fifty_move_count)
{
    return (fifty_move_count * 1024) / ((101 * 1024) / fifty_move_buckets);
}

constexpr uint64_t calculate_fifty_move_hash(int fifty_move_count)
{
    return fifty_move_bucket_hash[calculate_fifty_move_bucket(fifty_move_count)];
}

constexpr auto fifty_move_hash = []()
{
    std::array<uint64_t, 101> buckets = {};
    for (int i = 0; i <= 100; i++)
    {
        buckets[i] = calculate_fifty_move_hash(i);
    }
    return buckets;
}();

uint64_t get_fifty_move_adj_key(const BoardState& board)
{
    assert(0 <= board.fifty_move_count && board.fifty_move_count <= 100);
    return board.key ^ fifty_move_hash[board.fifty_move_count];
}

// This must mirror the incremental key updates in BoardState::apply_move and BoardState::update_castle_rights, and is
// verified against them by an assert at the end of apply_move.
uint64_t get_fifty_move_adj_key_after(const BoardState& board, Move move)
{
    uint64_t key = board.key ^ stm();

    // undo the previous ep square
    if (board.en_passant <= SQ_H8)
    {
        key ^= en_passant(enum_to<File>(board.en_passant));
    }

    // castle rights: check for the king or rook moving, or a rook being captured
    uint64_t castle_squares = board.castle_squares;

    if (move.from() == board.get_king_sq(WHITE))
    {
        uint64_t white_castle = castle_squares & RankBB[RANK_1];

        while (white_castle)
        {
            key ^= castle(lsbpop(white_castle));
        }

        castle_squares &= ~(RankBB[RANK_1]);
    }

    if (move.from() == board.get_king_sq(BLACK))
    {
        uint64_t black_castle = castle_squares & RankBB[RANK_8];

        while (black_castle)
        {
            key ^= castle(lsbpop(black_castle));
        }

        castle_squares &= ~(RankBB[RANK_8]);
    }

    if (SquareBB[move.to()] & castle_squares)
    {
        key ^= castle(move.to());
    }

    if (SquareBB[move.from()] & castle_squares)
    {
        key ^= castle(move.from());
    }

    switch (move.flag())
    {
    case QUIET:
    {
        const auto piece = board.get_square_piece(move.from());
        key ^= piece_square(piece, move.from());
        key ^= piece_square(piece, move.to());
        break;
    }
    case PAWN_DOUBLE_MOVE:
    {
        const auto piece = get_piece(PAWN, board.stm);
        key ^= piece_square(piece, move.from());
        key ^= piece_square(piece, move.to());

        // the new ep square only enters the key if the opponent has a legal ep capture
        Square ep_sq = static_cast<Square>((move.to() + move.from()) / 2);
        uint64_t potential_attackers = PawnAttacks[board.stm][ep_sq] & board.get_pieces_bb(PAWN, !board.stm);

        while (potential_attackers)
        {
            if (ep_is_legal(board, Move(lsbpop(potential_attackers), ep_sq, EN_PASSANT), !board.stm))
            {
                key ^= en_passant(enum_to<File>(ep_sq));
                break;
            }
        }

        break;
    }
    case A_SIDE_CASTLE:
    {
        Square king_end = board.stm == WHITE ? SQ_C1 : SQ_C8;
        Square rook_end = board.stm == WHITE ? SQ_D1 : SQ_D8;
        key ^= piece_square(get_piece(KING, board.stm), move.from());
        key ^= piece_square(get_piece(KING, board.stm), king_end);
        key ^= piece_square(get_piece(ROOK, board.stm), move.to());
        key ^= piece_square(get_piece(ROOK, board.stm), rook_end);
        break;
    }
    case H_SIDE_CASTLE:
    {
        Square king_end = board.stm == WHITE ? SQ_G1 : SQ_G8;
        Square rook_end = board.stm == WHITE ? SQ_F1 : SQ_F8;
        key ^= piece_square(get_piece(KING, board.stm), move.from());
        key ^= piece_square(get_piece(KING, board.stm), king_end);
        key ^= piece_square(get_piece(ROOK, board.stm), move.to());
        key ^= piece_square(get_piece(ROOK, board.stm), rook_end);
        break;
    }
    case CAPTURE:
    {
        const auto piece = board.get_square_piece(move.from());
        key ^= piece_square(piece, move.from());
        key ^= piece_square(piece, move.to());
        key ^= piece_square(board.get_square_piece(move.to()), move.to());
        break;
    }
    case EN_PASSANT:
    {
        const auto ep_cap_sq = get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()));
        const auto piece = get_piece(PAWN, board.stm);
        key ^= piece_square(get_piece(PAWN, !board.stm), ep_cap_sq);
        key ^= piece_square(piece, move.from());
        key ^= piece_square(piece, move.to());
        break;
    }
    default:
    {
        assert(move.is_promotion());
        static_assert(BISHOP_PROMOTION - KNIGHT_PROMOTION == BISHOP - KNIGHT);
        static_assert(QUEEN_PROMOTION_CAPTURE - KNIGHT_PROMOTION_CAPTURE == QUEEN - KNIGHT);
        const auto promo_type = static_cast<PieceType>(KNIGHT + ((move.flag() - KNIGHT_PROMOTION) & 3));
        key ^= piece_square(get_piece(PAWN, board.stm), move.from());
        key ^= piece_square(get_piece(promo_type, board.stm), move.to());
        if (move.is_capture())
        {
            key ^= piece_square(board.get_square_piece(move.to()), move.to());
        }
        break;
    }
    }

    const bool resets_fifty_move
        = move.is_capture() || move.is_promotion() || board.get_square_piece(move.from()) == get_piece(PAWN, board.stm);
    const auto new_fifty_move = resets_fifty_move ? 0 : board.fifty_move_count + 1;
    assert(0 <= new_fifty_move && new_fifty_move <= 100);
    return key ^ fifty_move_hash[new_fifty_move];
}

} // namespace Zobrist
