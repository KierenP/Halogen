#include "chessboard/board_state.h"

#include <cassert>
#include <charconv>
#include <cstddef>
#include <iostream>

#include "bitboard/define.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "search/zobrist.h"

BoardState::BoardState()
{
    mailbox.fill(N_PIECES);
}

bool BoardState::init_from_fen(const std::array<std::string_view, 6>& fen)
{
    *this = BoardState(); // reset the board state

    size_t FenLetter = 0; // index within the string
    int square = 0; // index within the board
    while ((square < 64) && (FenLetter < fen[0].length()))
    {
        char letter = fen[0].at(FenLetter);
        // ugly code, but surely this is the only time in the code that we iterate over squares in a different order
        // than the Square enum the squares values go up as you move up the board towards black's starting side, but a
        // fen starts from the black side and works downwards
        Square sq = static_cast<Square>((RANK_8 - square / 8) * 8 + square % 8);

        // clang-format off
		switch (letter)
		{
		case 'p': add_piece_sq(sq, BLACK_PAWN); break;
		case 'r': add_piece_sq(sq, BLACK_ROOK); break;
		case 'n': add_piece_sq(sq, BLACK_KNIGHT); break;
		case 'b': add_piece_sq(sq, BLACK_BISHOP); break;
		case 'q': add_piece_sq(sq, BLACK_QUEEN); break;
		case 'k': add_piece_sq(sq, BLACK_KING); break;
		case 'P': add_piece_sq(sq, WHITE_PAWN); break;
		case 'R': add_piece_sq(sq, WHITE_ROOK); break;
		case 'N': add_piece_sq(sq, WHITE_KNIGHT); break;
		case 'B': add_piece_sq(sq, WHITE_BISHOP); break;
		case 'Q': add_piece_sq(sq, WHITE_QUEEN); break;
		case 'K': add_piece_sq(sq, WHITE_KING); break;
		case '/': square--; break;
		case '1': break;
		case '2': square += 1; break;
		case '3': square += 2; break;
		case '4': square += 3; break;
		case '5': square += 4; break;
		case '6': square += 5; break;
		case '7': square += 6; break;
		case '8': square += 7; break;
		default: return false;					//bad fen
		}
        // clang-format on
        square++;
        FenLetter++;
    }

    if (fen[1] == "w")
        stm = WHITE;
    else if (fen[1] == "b")
        stm = BLACK;
    else
        return false;

    for (auto& letter : fen[2])
    {
        if (letter == '-')
            continue;

        // parse classic fen or chess960 fen (KQkq)
        if (letter == 'K')
            castle_squares |= SquareBB[msb(get_pieces_bb(WHITE_ROOK) & RankBB[RANK_1])];

        else if (letter == 'Q')
            castle_squares |= SquareBB[lsb(get_pieces_bb(WHITE_ROOK) & RankBB[RANK_1])];

        else if (letter == 'k')
            castle_squares |= SquareBB[msb(get_pieces_bb(BLACK_ROOK) & RankBB[RANK_8])];

        else if (letter == 'q')
            castle_squares |= SquareBB[lsb(get_pieces_bb(BLACK_ROOK) & RankBB[RANK_8])];

        // parse Shredder-FEN (HAha)
        else if (letter >= 'A' && letter <= 'H')
            castle_squares |= SquareBB[letter - 'A'];

        else if (letter >= 'a' && letter <= 'h')
            castle_squares |= SquareBB[SQ_A8 + letter - 'a'];

        else
            return false;
    }

    if (fen[3] == "-")
    {
        en_passant = N_SQUARES;
    }
    else
    {
        if (fen[3].length() != 2)
            return false;

        en_passant = static_cast<Square>((fen[3][0] - 'a') + (fen[3][1] - '1') * 8);
    };

    auto [ptr1, ec1] = std::from_chars(fen[4].begin(), fen[4].end(), fifty_move_count);
    if (ptr1 != fen[4].end())
    {
        return false;
    }

    int full_move_count = 0;
    auto [ptr2, ec2] = std::from_chars(fen[5].begin(), fen[5].end(), full_move_count);
    if (ptr2 != fen[5].end())
    {
        return false;
    }

    half_turn_count = full_move_count * 2 - (stm == WHITE); // convert full move count to half move count

    recalculate_side_bb();
    key = Zobrist::key(*this);
    pawn_key = Zobrist::pawn_key(*this);
    non_pawn_key[WHITE] = Zobrist::non_pawn_key(*this, WHITE);
    non_pawn_key[BLACK] = Zobrist::non_pawn_key(*this, BLACK);
    // minor_key = Zobrist::minor_key(*this);
    // major_key = Zobrist::major_key(*this);
    update_lesser_threats();
    return true;
}

void BoardState::recalculate_side_bb()
{
    side_bb[WHITE] = get_pieces_bb(WHITE_PAWN) | get_pieces_bb(WHITE_KNIGHT) | get_pieces_bb(WHITE_BISHOP)
        | get_pieces_bb(WHITE_ROOK) | get_pieces_bb(WHITE_QUEEN) | get_pieces_bb(WHITE_KING);
    side_bb[BLACK] = get_pieces_bb(BLACK_PAWN) | get_pieces_bb(BLACK_KNIGHT) | get_pieces_bb(BLACK_BISHOP)
        | get_pieces_bb(BLACK_ROOK) | get_pieces_bb(BLACK_QUEEN) | get_pieces_bb(BLACK_KING);
}

uint64_t BoardState::get_pieces_bb(Side colour) const
{
    return side_bb[colour];
}

void BoardState::add_piece_sq(Square square, Piece piece)
{
    assert(square < N_SQUARES);
    assert(piece < N_PIECES);

    board[piece] |= SquareBB[square];
    side_bb[enum_to<Side>(piece)] |= SquareBB[square];
    mailbox[square] = piece;
}

void BoardState::remove_piece_sq(Square square, Piece piece)
{
    assert(square < N_SQUARES);
    assert(piece < N_PIECES);

    board[piece] ^= SquareBB[square];
    side_bb[enum_to<Side>(piece)] ^= SquareBB[square];
    mailbox[square] = N_PIECES;
}

uint64_t BoardState::get_pieces_bb(Piece piece) const
{
    return board[piece];
}

uint64_t BoardState::get_pieces_bb(PieceType type) const
{
    return get_pieces_bb(type, WHITE) | get_pieces_bb(type, BLACK);
}

void BoardState::clear_sq(Square square)
{
    assert(square < N_SQUARES);

    for (int i = 0; i < N_PIECES; i++)
    {
        board[i] &= ~SquareBB[square];
    }

    side_bb[WHITE] &= ~SquareBB[square];
    side_bb[BLACK] &= ~SquareBB[square];
    mailbox[square] = N_PIECES;
}

uint64_t BoardState::get_pieces_bb(PieceType pieceType, Side colour) const
{
    return get_pieces_bb(get_piece(pieceType, colour));
}

Square BoardState::get_king_sq(Side colour) const
{
    assert(get_pieces_bb(KING, colour) != 0);
    return lsb(get_pieces_bb(KING, colour));
}

bool BoardState::is_empty(Square square) const
{
    assert(square != N_SQUARES);
    return ((get_pieces_bb() & SquareBB[square]) == 0);
}

bool BoardState::is_occupied(Square square) const
{
    assert(square != N_SQUARES);
    return (!is_empty(square));
}

Piece BoardState::get_square_piece(Square square) const
{
    return mailbox[square];
}

uint64_t BoardState::get_pieces_bb() const
{
    return get_pieces_bb(WHITE) | get_pieces_bb(BLACK);
}

uint64_t BoardState::get_empty_bb() const
{
    return ~get_pieces_bb();
}

void BoardState::update_castle_rights(Move move)
{
    // check for the king or rook moving, or a rook being captured

    if (move.from() == get_king_sq(WHITE))
    {
        // get castle squares on white side
        uint64_t white_castle = castle_squares & RankBB[RANK_1];

        while (white_castle)
        {
            key ^= Zobrist::castle(lsbpop(white_castle));
        }

        castle_squares &= ~(RankBB[RANK_1]);
    }

    if (move.from() == get_king_sq(BLACK))
    {
        // get castle squares on black side
        uint64_t black_castle = castle_squares & RankBB[RANK_8];

        while (black_castle)
        {
            key ^= Zobrist::castle(lsbpop(black_castle));
        }

        castle_squares &= ~(RankBB[RANK_8]);
    }

    if (SquareBB[move.to()] & castle_squares)
    {
        key ^= Zobrist::castle(move.to());
        castle_squares &= ~SquareBB[move.to()];
    }

    if (SquareBB[move.from()] & castle_squares)
    {
        key ^= Zobrist::castle(move.from());
        castle_squares &= ~SquareBB[move.from()];
    }
}

std::ostream& operator<<(std::ostream& os, const BoardState& b)
{
    os << "  A B C D E F G H";

    std::array<char, N_SQUARES> Letter {};

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        constexpr static std::array PieceChar = { 'p', 'n', 'b', 'r', 'q', 'k', 'P', 'N', 'B', 'R', 'Q', 'K', ' ' };
        Letter[i] = PieceChar[b.get_square_piece(i)];
    }

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        auto square = get_square(enum_to<File>(i), mirror(enum_to<Rank>(i)));

        if (enum_to<File>(square) == FILE_A)
        {
            os << "\n";
            os << 8 - enum_to<Rank>(i);
        }

        os << " ";
        os << Letter[square];
    }

    os << "\n";
    os << "en_passant: " << b.en_passant << "\n";
    os << "fifty_move_count: " << b.fifty_move_count << "\n";
    os << "half_turn_count: " << b.half_turn_count << "\n";
    os << "stm: " << b.stm << "\n";
    os << "castle_squares: " << b.castle_squares << "\n";

    return os;
}

void BoardState::apply_move(Move move)
{
    // std::cout << *this << std::endl;

    key ^= Zobrist::stm();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
    {
        key ^= Zobrist::en_passant(enum_to<File>(en_passant));
    }

    en_passant = N_SQUARES;

    update_castle_rights(move);

    switch (move.flag())
    {
    case QUIET:
    {
        const auto piece = get_square_piece(move.from());

        add_piece_sq(move.to(), piece);
        remove_piece_sq(move.from(), piece);

        key ^= Zobrist::piece_square(piece, move.from());
        key ^= Zobrist::piece_square(piece, move.to());
        if (enum_to<PieceType>(piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(piece, move.from());
            pawn_key ^= Zobrist::piece_square(piece, move.to());
        }
        else
        {
            non_pawn_key[stm] ^= Zobrist::piece_square(piece, move.from());
            non_pawn_key[stm] ^= Zobrist::piece_square(piece, move.to());
            if (enum_to<PieceType>(piece) == KNIGHT || enum_to<PieceType>(piece) == BISHOP)
            {
                // minor_key ^= Zobrist::piece_square(piece, move.GetFrom());
                // minor_key ^= Zobrist::piece_square(piece, move.to());
            }
            else if (enum_to<PieceType>(piece) == ROOK || enum_to<PieceType>(piece) == QUEEN)
            {
                // major_key ^= Zobrist::piece_square(piece, move.GetFrom());
                // major_key ^= Zobrist::piece_square(piece, move.to());
            }
        }

        break;
    }
    case PAWN_DOUBLE_MOVE:
    {
        // average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the
        // same for black and white
        Square ep_sq = static_cast<Square>((move.to() + move.from()) / 2);

        // it only counts as a ep square if a pawn could potentially do the capture!
        uint64_t potentialAttackers = PawnAttacks[stm][ep_sq] & get_pieces_bb(PAWN, !stm);

        while (potentialAttackers)
        {
            stm = !stm;
            bool legal = ep_is_legal(*this, Move(lsbpop(potentialAttackers), ep_sq, EN_PASSANT));
            stm = !stm;
            if (legal)
            {
                en_passant = ep_sq;
                key ^= Zobrist::en_passant(enum_to<File>(ep_sq));
                break;
            }
        }

        const auto piece = get_piece(PAWN, stm);

        add_piece_sq(move.to(), piece);
        remove_piece_sq(move.from(), piece);

        key ^= Zobrist::piece_square(piece, move.from());
        key ^= Zobrist::piece_square(piece, move.to());
        pawn_key ^= Zobrist::piece_square(piece, move.from());
        pawn_key ^= Zobrist::piece_square(piece, move.to());

        break;
    }
    case A_SIDE_CASTLE:
    {
        Square king_start = move.from();
        Square king_end = stm == WHITE ? SQ_C1 : SQ_C8;
        Square rook_start = move.to();
        Square rook_end = stm == WHITE ? SQ_D1 : SQ_D8;

        remove_piece_sq(king_start, get_piece(KING, stm));
        remove_piece_sq(rook_start, get_piece(ROOK, stm));
        add_piece_sq(king_end, get_piece(KING, stm));
        add_piece_sq(rook_end, get_piece(ROOK, stm));

        key ^= Zobrist::piece_square(get_piece(KING, stm), king_start);
        key ^= Zobrist::piece_square(get_piece(KING, stm), king_end);
        key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(KING, stm), king_start);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(KING, stm), king_end);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);
        // major_key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        // major_key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);

        break;
    }
    case H_SIDE_CASTLE:
    {
        Square king_start = move.from();
        Square king_end = stm == WHITE ? SQ_G1 : SQ_G8;
        Square rook_start = move.to();
        Square rook_end = stm == WHITE ? SQ_F1 : SQ_F8;

        remove_piece_sq(king_start, get_piece(KING, stm));
        remove_piece_sq(rook_start, get_piece(ROOK, stm));
        add_piece_sq(king_end, get_piece(KING, stm));
        add_piece_sq(rook_end, get_piece(ROOK, stm));

        key ^= Zobrist::piece_square(get_piece(KING, stm), king_start);
        key ^= Zobrist::piece_square(get_piece(KING, stm), king_end);
        key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(KING, stm), king_start);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(KING, stm), king_end);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        non_pawn_key[stm] ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);
        // major_key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_start);
        // major_key ^= Zobrist::piece_square(get_piece(ROOK, stm), rook_end);

        break;
    }
    case CAPTURE:
    {
        const auto cap_piece = get_square_piece(move.to());
        const auto piece = get_square_piece(move.from());

        remove_piece_sq(move.to(), cap_piece);
        add_piece_sq(move.to(), piece);
        remove_piece_sq(move.from(), piece);

        key ^= Zobrist::piece_square(piece, move.from());
        key ^= Zobrist::piece_square(piece, move.to());
        if (enum_to<PieceType>(piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(piece, move.from());
            pawn_key ^= Zobrist::piece_square(piece, move.to());
        }
        else
        {
            non_pawn_key[stm] ^= Zobrist::piece_square(piece, move.from());
            non_pawn_key[stm] ^= Zobrist::piece_square(piece, move.to());
            if (enum_to<PieceType>(piece) == KNIGHT || enum_to<PieceType>(piece) == BISHOP)
            {
                // minor_key ^= Zobrist::piece_square(piece, move.GetFrom());
                // minor_key ^= Zobrist::piece_square(piece, move.to());
            }
            else if (enum_to<PieceType>(piece) == ROOK || enum_to<PieceType>(piece) == QUEEN)
            {
                // major_key ^= Zobrist::piece_square(piece, move.GetFrom());
                // major_key ^= Zobrist::piece_square(piece, move.to());
            }
        }
        key ^= Zobrist::piece_square(cap_piece, move.to());
        if (enum_to<PieceType>(cap_piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(cap_piece, move.to());
        }
        else
        {
            non_pawn_key[!stm] ^= Zobrist::piece_square(cap_piece, move.to());
            if (enum_to<PieceType>(cap_piece) == KNIGHT || enum_to<PieceType>(cap_piece) == BISHOP)
            {
                // minor_key ^= Zobrist::piece_square(cap_piece, move.to());
            }
            else if (enum_to<PieceType>(cap_piece) == ROOK || enum_to<PieceType>(cap_piece) == QUEEN)
            {
                // major_key ^= Zobrist::piece_square(cap_piece, move.to());
            }
        }

        break;
    }
    case EN_PASSANT:
    {
        const auto ep_cap_sq = get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()));
        const auto cap_piece = get_piece(PAWN, !stm);
        const auto piece = get_piece(PAWN, stm);

        remove_piece_sq(ep_cap_sq, cap_piece);
        add_piece_sq(move.to(), piece);
        remove_piece_sq(move.from(), piece);

        key ^= Zobrist::piece_square(cap_piece, ep_cap_sq);
        key ^= Zobrist::piece_square(piece, move.from());
        key ^= Zobrist::piece_square(piece, move.to());

        pawn_key ^= Zobrist::piece_square(cap_piece, ep_cap_sq);
        pawn_key ^= Zobrist::piece_square(piece, move.from());
        pawn_key ^= Zobrist::piece_square(piece, move.to());

        break;
    }
    case KNIGHT_PROMOTION:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(KNIGHT, stm);

        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // minor_key ^= Zobrist::piece_square(promo_piece, move.to());

        break;
    }
    case BISHOP_PROMOTION:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(BISHOP, stm);

        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // minor_key ^= Zobrist::piece_square(promo_piece, move.to());

        break;
    }
    case ROOK_PROMOTION:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(ROOK, stm);

        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // major_key ^= Zobrist::piece_square(promo_piece, move.to());

        break;
    }
    case QUEEN_PROMOTION:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(QUEEN, stm);

        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // major_key ^= Zobrist::piece_square(promo_piece, move.to());

        break;
    }
    case KNIGHT_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(KNIGHT, stm);
        const auto cap_piece = get_square_piece(move.to());

        remove_piece_sq(move.to(), cap_piece);
        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // minor_key ^= Zobrist::piece_square(promo_piece, move.to());
        key ^= Zobrist::piece_square(cap_piece, move.to());
        non_pawn_key[!stm] ^= Zobrist::piece_square(cap_piece, move.to());
        if (enum_to<PieceType>(cap_piece) == KNIGHT || enum_to<PieceType>(cap_piece) == BISHOP)
        {
            // minor_key ^= Zobrist::piece_square(cap_piece, move.to());
        }
        else if (enum_to<PieceType>(cap_piece) == ROOK || enum_to<PieceType>(cap_piece) == QUEEN)
        {
            // major_key ^= Zobrist::piece_square(cap_piece, move.to());
        }

        break;
    }
    case BISHOP_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(BISHOP, stm);
        const auto cap_piece = get_square_piece(move.to());

        remove_piece_sq(move.to(), cap_piece);
        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // minor_key ^= Zobrist::piece_square(promo_piece, move.to());
        key ^= Zobrist::piece_square(cap_piece, move.to());
        non_pawn_key[!stm] ^= Zobrist::piece_square(cap_piece, move.to());
        if (enum_to<PieceType>(cap_piece) == KNIGHT || enum_to<PieceType>(cap_piece) == BISHOP)
        {
            // minor_key ^= Zobrist::piece_square(cap_piece, move.to());
        }
        else if (enum_to<PieceType>(cap_piece) == ROOK || enum_to<PieceType>(cap_piece) == QUEEN)
        {
            // major_key ^= Zobrist::piece_square(cap_piece, move.to());
        }

        break;
    }
    case ROOK_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(ROOK, stm);
        const auto cap_piece = get_square_piece(move.to());

        remove_piece_sq(move.to(), cap_piece);
        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // major_key ^= Zobrist::piece_square(promo_piece, move.to());
        key ^= Zobrist::piece_square(cap_piece, move.to());
        non_pawn_key[!stm] ^= Zobrist::piece_square(cap_piece, move.to());
        if (enum_to<PieceType>(cap_piece) == KNIGHT || enum_to<PieceType>(cap_piece) == BISHOP)
        {
            // minor_key ^= Zobrist::piece_square(cap_piece, move.to());
        }
        else if (enum_to<PieceType>(cap_piece) == ROOK || enum_to<PieceType>(cap_piece) == QUEEN)
        {
            // major_key ^= Zobrist::piece_square(cap_piece, move.to());
        }

        break;
    }
    case QUEEN_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = get_piece(PAWN, stm);
        const auto promo_piece = get_piece(QUEEN, stm);
        const auto cap_piece = get_square_piece(move.to());

        remove_piece_sq(move.to(), cap_piece);
        add_piece_sq(move.to(), promo_piece);
        remove_piece_sq(move.from(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.from());
        key ^= Zobrist::piece_square(promo_piece, move.to());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.from());
        non_pawn_key[stm] ^= Zobrist::piece_square(promo_piece, move.to());
        // major_key ^= Zobrist::piece_square(promo_piece, move.to());
        key ^= Zobrist::piece_square(cap_piece, move.to());
        non_pawn_key[!stm] ^= Zobrist::piece_square(cap_piece, move.to());
        if (enum_to<PieceType>(cap_piece) == KNIGHT || enum_to<PieceType>(cap_piece) == BISHOP)
        {
            // minor_key ^= Zobrist::piece_square(cap_piece, move.to());
        }
        else if (enum_to<PieceType>(cap_piece) == ROOK || enum_to<PieceType>(cap_piece) == QUEEN)
        {
            // major_key ^= Zobrist::piece_square(cap_piece, move.to());
        }

        break;
    }
    default:
        assert(0);
    }

    if (move.is_capture() || get_square_piece(move.to()) == get_piece(PAWN, stm) || move.is_promotion())
        fifty_move_count = 0;
    else
        fifty_move_count++;

    half_turn_count += 1;
    stm = !stm;

    // std::cout << *this << std::endl;
    // std::cout << move << std::endl;

    assert(key == Zobrist::key(*this));
    assert(pawn_key == Zobrist::pawn_key(*this));
    assert(non_pawn_key[WHITE] == Zobrist::non_pawn_key(*this, WHITE));
    assert(non_pawn_key[BLACK] == Zobrist::non_pawn_key(*this, BLACK));
    // assert(minor_key == Zobrist::minor_key(*this));
    // assert(major_key == Zobrist::major_key(*this));
    update_lesser_threats();
}

void BoardState::apply_null_move()
{
    key ^= Zobrist::stm();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
    {
        key ^= Zobrist::en_passant(enum_to<File>(en_passant));
    }

    en_passant = N_SQUARES;
    fifty_move_count++;
    half_turn_count += 1;
    stm = !stm;

    assert(key == Zobrist::key(*this));
    assert(pawn_key == Zobrist::pawn_key(*this));
    assert(non_pawn_key[WHITE] == Zobrist::non_pawn_key(*this, WHITE));
    assert(non_pawn_key[BLACK] == Zobrist::non_pawn_key(*this, BLACK));
    // assert(minor_key == Zobrist::minor_key(*this));
    // assert(major_key == Zobrist::major_key(*this));
    update_lesser_threats();
}

MoveFlag BoardState::infer_move_flag(Square from, Square to) const
{
    // Castling (normal chess)
    if ((from == SQ_E1 && to == SQ_G1 && get_square_piece(from) == WHITE_KING)
        || (from == SQ_E8 && to == SQ_G8 && get_square_piece(from) == BLACK_KING))
    {
        return H_SIDE_CASTLE;
    }

    if ((from == SQ_E1 && to == SQ_C1 && get_square_piece(from) == WHITE_KING)
        || (from == SQ_E8 && to == SQ_C8 && get_square_piece(from) == BLACK_KING))
    {
        return A_SIDE_CASTLE;
    }

    // Castling (chess960). Needs to be handled before the 'capture' case
    if ((get_square_piece(from) == WHITE_KING && get_square_piece(to) == WHITE_ROOK)
        || (get_square_piece(from) == BLACK_KING && get_square_piece(to) == BLACK_ROOK))
    {
        if (from > to)
        {
            return A_SIDE_CASTLE;
        }
        else
        {
            return H_SIDE_CASTLE;
        }
    }

    // Captures
    if (is_occupied(to))
    {
        return CAPTURE;
    }

    // Double pawn moves
    if (abs_rank_diff(from, to) == 2 && (get_square_piece(from) == WHITE_PAWN || get_square_piece(from) == BLACK_PAWN))
    {
        return PAWN_DOUBLE_MOVE;
    }

    // En passant
    if (to == en_passant && (get_square_piece(from) == WHITE_PAWN || get_square_piece(from) == BLACK_PAWN))
    {
        return EN_PASSANT;
    }

    return QUIET;
}

uint64_t BoardState::active_lesser_threats() const
{
    return (lesser_threats[KNIGHT] & get_pieces_bb(KNIGHT, stm)) | (lesser_threats[BISHOP] & get_pieces_bb(BISHOP, stm))
        | (lesser_threats[ROOK] & get_pieces_bb(ROOK, stm)) | (lesser_threats[QUEEN] & get_pieces_bb(QUEEN, stm))
        | (lesser_threats[KING] & get_pieces_bb(KING, stm));
}

void BoardState::update_lesser_threats()
{
    const uint64_t occ = get_pieces_bb();

    uint64_t attacks = EMPTY;

    // pawn threats
    for (uint64_t pieces = get_pieces_bb(PAWN, !stm); pieces != 0;)
    {
        attacks |= PawnAttacks[!stm][lsbpop(pieces)];
    }

    lesser_threats[KNIGHT] = attacks;

    // knight threats
    for (uint64_t pieces = get_pieces_bb(KNIGHT, !stm); pieces != 0;)
    {
        attacks |= attack_bb<KNIGHT>(lsbpop(pieces), occ);
    }

    lesser_threats[BISHOP] = attacks;

    // bishop threats
    for (uint64_t pieces = get_pieces_bb(BISHOP, !stm); pieces != 0;)
    {
        attacks |= attack_bb<BISHOP>(lsbpop(pieces), occ);
    }

    lesser_threats[ROOK] = attacks;

    // rook threats
    for (uint64_t pieces = get_pieces_bb(ROOK, !stm); pieces != 0;)
    {
        attacks |= attack_bb<ROOK>(lsbpop(pieces), occ);
    }

    lesser_threats[QUEEN] = attacks;

    // queen threats
    for (uint64_t pieces = get_pieces_bb(QUEEN, !stm); pieces != 0;)
    {
        attacks |= attack_bb<QUEEN>(lsbpop(pieces), occ);
    }

    lesser_threats[KING] = attacks;
}
