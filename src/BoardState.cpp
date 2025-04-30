#include "BoardState.h"

#include <cassert>
#include <charconv>
#include <cstddef>
#include <iostream>

#include "BitBoardDefine.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "Zobrist.h"

BoardState::BoardState()
{
    Reset();
}

void BoardState::Reset()
{
    en_passant = N_SQUARES;
    fifty_move_count = 0;
    half_turn_count = 1;

    stm = N_PLAYERS;
    castle_squares = EMPTY;

    board = {};

    RecalculateWhiteBlackBoards();
    key = Zobrist::key(*this);
    pawn_key = Zobrist::pawn_key(*this);
}

bool BoardState::InitialiseFromFen(const std::array<std::string_view, 6>& fen)
{
    Reset();

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
		case 'p': AddPiece(sq, BLACK_PAWN); break;
		case 'r': AddPiece(sq, BLACK_ROOK); break;
		case 'n': AddPiece(sq, BLACK_KNIGHT); break;
		case 'b': AddPiece(sq, BLACK_BISHOP); break;
		case 'q': AddPiece(sq, BLACK_QUEEN); break;
		case 'k': AddPiece(sq, BLACK_KING); break;
		case 'P': AddPiece(sq, WHITE_PAWN); break;
		case 'R': AddPiece(sq, WHITE_ROOK); break;
		case 'N': AddPiece(sq, WHITE_KNIGHT); break;
		case 'B': AddPiece(sq, WHITE_BISHOP); break;
		case 'Q': AddPiece(sq, WHITE_QUEEN); break;
		case 'K': AddPiece(sq, WHITE_KING); break;
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
            castle_squares |= SquareBB[MSB(GetPieceBB<WHITE_ROOK>() & RankBB[RANK_1])];

        else if (letter == 'Q')
            castle_squares |= SquareBB[LSB(GetPieceBB<WHITE_ROOK>() & RankBB[RANK_1])];

        else if (letter == 'k')
            castle_squares |= SquareBB[MSB(GetPieceBB<BLACK_ROOK>() & RankBB[RANK_8])];

        else if (letter == 'q')
            castle_squares |= SquareBB[LSB(GetPieceBB<BLACK_ROOK>() & RankBB[RANK_8])];

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

    RecalculateWhiteBlackBoards();
    key = Zobrist::key(*this);
    pawn_key = Zobrist::pawn_key(*this);
    return true;
}

void BoardState::RecalculateWhiteBlackBoards()
{
    side_bb[WHITE] = GetPieceBB<WHITE_PAWN>() | GetPieceBB<WHITE_KNIGHT>() | GetPieceBB<WHITE_BISHOP>()
        | GetPieceBB<WHITE_ROOK>() | GetPieceBB<WHITE_QUEEN>() | GetPieceBB<WHITE_KING>();
    side_bb[BLACK] = GetPieceBB<BLACK_PAWN>() | GetPieceBB<BLACK_KNIGHT>() | GetPieceBB<BLACK_BISHOP>()
        | GetPieceBB<BLACK_ROOK>() | GetPieceBB<BLACK_QUEEN>() | GetPieceBB<BLACK_KING>();
}

uint64_t BoardState::GetPiecesColour(Players colour) const
{
    if (colour == WHITE)
        return GetPieces<WHITE>();
    else
        return GetPieces<BLACK>();
}

uint64_t BoardState::GetZobristKey() const
{
    return key;
}

uint64_t BoardState::GetPawnKey() const
{
    return pawn_key;
}

void BoardState::AddPiece(Square square, Pieces piece)
{
    assert(square < N_SQUARES);
    assert(piece < N_PIECES);

    board[piece] |= SquareBB[square];
    side_bb[ColourOfPiece(piece)] |= SquareBB[square];
}

void BoardState::RemovePiece(Square square, Pieces piece)
{
    assert(square < N_SQUARES);
    assert(piece < N_PIECES);

    board[piece] ^= SquareBB[square];
    side_bb[ColourOfPiece(piece)] ^= SquareBB[square];
}

uint64_t BoardState::GetPieceBB(Pieces piece) const
{
    return board[piece];
}

void BoardState::ClearSquare(Square square)
{
    assert(square < N_SQUARES);

    for (int i = 0; i < N_PIECES; i++)
    {
        board[i] &= ~SquareBB[square];
    }

    side_bb[WHITE] &= ~SquareBB[square];
    side_bb[BLACK] &= ~SquareBB[square];
}

uint64_t BoardState::GetPieceBB(PieceTypes pieceType, Players colour) const
{
    return GetPieceBB(Piece(pieceType, colour));
}

Square BoardState::GetKing(Players colour) const
{
    assert(GetPieceBB(KING, colour) != 0);
    return LSB(GetPieceBB(KING, colour));
}

bool BoardState::IsEmpty(Square square) const
{
    assert(square != N_SQUARES);
    return ((GetAllPieces() & SquareBB[square]) == 0);
}

bool BoardState::IsOccupied(Square square) const
{
    assert(square != N_SQUARES);
    return (!IsEmpty(square));
}

Pieces BoardState::GetSquare(Square square) const
{
    for (int i = 0; i < N_PIECES; i++)
    {
        if ((GetPieceBB(static_cast<Pieces>(i)) & SquareBB[square]) != 0)
            return static_cast<Pieces>(i);
    }

    return N_PIECES;
}

uint64_t BoardState::GetAllPieces() const
{
    return GetPieces<WHITE>() | GetPieces<BLACK>();
}

uint64_t BoardState::GetEmptySquares() const
{
    return ~GetAllPieces();
}

void BoardState::UpdateCastleRights(Move move)
{
    // check for the king or rook moving, or a rook being captured

    if (move.GetFrom() == GetKing(WHITE))
    {
        // get castle squares on white side
        uint64_t white_castle = castle_squares & RankBB[RANK_1];

        while (white_castle)
        {
            key ^= Zobrist::castle(LSBpop(white_castle));
        }

        castle_squares &= ~(RankBB[RANK_1]);
    }

    if (move.GetFrom() == GetKing(BLACK))
    {
        // get castle squares on black side
        uint64_t black_castle = castle_squares & RankBB[RANK_8];

        while (black_castle)
        {
            key ^= Zobrist::castle(LSBpop(black_castle));
        }

        castle_squares &= ~(RankBB[RANK_8]);
    }

    if (SquareBB[move.GetTo()] & castle_squares)
    {
        key ^= Zobrist::castle(move.GetTo());
        castle_squares &= ~SquareBB[move.GetTo()];
    }

    if (SquareBB[move.GetFrom()] & castle_squares)
    {
        key ^= Zobrist::castle(move.GetFrom());
        castle_squares &= ~SquareBB[move.GetFrom()];
    }
}

std::ostream& operator<<(std::ostream& os, const BoardState& b)
{
    os << "  A B C D E F G H";

    char Letter[N_SQUARES];

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        constexpr static char PieceChar[13] = { 'p', 'n', 'b', 'r', 'q', 'k', 'P', 'N', 'B', 'R', 'Q', 'K', ' ' };
        Letter[i] = PieceChar[b.GetSquare(i)];
    }

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        auto square = GetPosition(GetFile(i), Mirror(GetRank(i)));

        if (GetFile(square) == FILE_A)
        {
            os << "\n";
            os << 8 - GetRank(i);
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

void BoardState::ApplyMove(Move move)
{
    key ^= Zobrist::stm();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
    {
        key ^= Zobrist::en_passant(GetFile(en_passant));
    }

    en_passant = N_SQUARES;

    UpdateCastleRights(move);

    switch (move.GetFlag())
    {
    case QUIET:
    {
        const auto piece = GetSquare(move.GetFrom());

        AddPiece(move.GetTo(), piece);
        RemovePiece(move.GetFrom(), piece);

        key ^= Zobrist::piece_square(piece, move.GetFrom());
        key ^= Zobrist::piece_square(piece, move.GetTo());
        if (GetPieceType(piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(piece, move.GetFrom());
            pawn_key ^= Zobrist::piece_square(piece, move.GetTo());
        }

        break;
    }
    case PAWN_DOUBLE_MOVE:
    {
        // average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the
        // same for black and white
        Square ep_sq = static_cast<Square>((move.GetTo() + move.GetFrom()) / 2);

        // it only counts as a ep square if a pawn could potentially do the capture!
        uint64_t potentialAttackers = PawnAttacks[stm][ep_sq] & GetPieceBB(PAWN, !stm);

        while (potentialAttackers)
        {
            stm = !stm;
            bool legal = EnPassantIsLegal(*this, Move(LSBpop(potentialAttackers), ep_sq, EN_PASSANT));
            stm = !stm;
            if (legal)
            {
                en_passant = ep_sq;
                key ^= Zobrist::en_passant(GetFile(ep_sq));
                break;
            }
        }

        const auto piece = Piece(PAWN, stm);

        AddPiece(move.GetTo(), piece);
        RemovePiece(move.GetFrom(), piece);

        key ^= Zobrist::piece_square(piece, move.GetFrom());
        key ^= Zobrist::piece_square(piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(piece, move.GetFrom());
        pawn_key ^= Zobrist::piece_square(piece, move.GetTo());

        break;
    }
    case A_SIDE_CASTLE:
    {
        Square king_start = move.GetFrom();
        Square king_end = stm == WHITE ? SQ_C1 : SQ_C8;
        Square rook_start = move.GetTo();
        Square rook_end = stm == WHITE ? SQ_D1 : SQ_D8;

        RemovePiece(king_start, Piece(KING, stm));
        RemovePiece(rook_start, Piece(ROOK, stm));
        AddPiece(king_end, Piece(KING, stm));
        AddPiece(rook_end, Piece(ROOK, stm));

        key ^= Zobrist::piece_square(Piece(KING, stm), king_start);
        key ^= Zobrist::piece_square(Piece(KING, stm), king_end);
        key ^= Zobrist::piece_square(Piece(ROOK, stm), rook_start);
        key ^= Zobrist::piece_square(Piece(ROOK, stm), rook_end);

        break;
    }
    case H_SIDE_CASTLE:
    {
        Square king_start = move.GetFrom();
        Square king_end = stm == WHITE ? SQ_G1 : SQ_G8;
        Square rook_start = move.GetTo();
        Square rook_end = stm == WHITE ? SQ_F1 : SQ_F8;

        RemovePiece(king_start, Piece(KING, stm));
        RemovePiece(rook_start, Piece(ROOK, stm));
        AddPiece(king_end, Piece(KING, stm));
        AddPiece(rook_end, Piece(ROOK, stm));

        key ^= Zobrist::piece_square(Piece(KING, stm), king_start);
        key ^= Zobrist::piece_square(Piece(KING, stm), king_end);
        key ^= Zobrist::piece_square(Piece(ROOK, stm), rook_start);
        key ^= Zobrist::piece_square(Piece(ROOK, stm), rook_end);

        break;
    }
    case CAPTURE:
    {
        const auto cap_piece = GetSquare(move.GetTo());
        const auto piece = GetSquare(move.GetFrom());

        RemovePiece(move.GetTo(), cap_piece);
        AddPiece(move.GetTo(), piece);
        RemovePiece(move.GetFrom(), piece);

        key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        key ^= Zobrist::piece_square(piece, move.GetFrom());
        key ^= Zobrist::piece_square(piece, move.GetTo());
        if (GetPieceType(piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(piece, move.GetFrom());
            pawn_key ^= Zobrist::piece_square(piece, move.GetTo());
        }
        if (GetPieceType(cap_piece) == PAWN)
        {
            pawn_key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        }

        break;
    }
    case EN_PASSANT:
    {
        const auto ep_cap_sq = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));
        const auto cap_piece = Piece(PAWN, !stm);
        const auto piece = Piece(PAWN, stm);

        RemovePiece(ep_cap_sq, cap_piece);
        AddPiece(move.GetTo(), piece);
        RemovePiece(move.GetFrom(), piece);

        key ^= Zobrist::piece_square(cap_piece, ep_cap_sq);
        key ^= Zobrist::piece_square(piece, move.GetFrom());
        key ^= Zobrist::piece_square(piece, move.GetTo());

        pawn_key ^= Zobrist::piece_square(cap_piece, ep_cap_sq);
        pawn_key ^= Zobrist::piece_square(piece, move.GetFrom());
        pawn_key ^= Zobrist::piece_square(piece, move.GetTo());

        break;
    }
    case KNIGHT_PROMOTION:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(KNIGHT, stm);

        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case BISHOP_PROMOTION:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(BISHOP, stm);

        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case ROOK_PROMOTION:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(ROOK, stm);

        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case QUEEN_PROMOTION:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(QUEEN, stm);

        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case KNIGHT_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(KNIGHT, stm);
        const auto cap_piece = GetSquare(move.GetTo());

        RemovePiece(move.GetTo(), cap_piece);
        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case BISHOP_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(BISHOP, stm);
        const auto cap_piece = GetSquare(move.GetTo());

        RemovePiece(move.GetTo(), cap_piece);
        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case ROOK_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(ROOK, stm);
        const auto cap_piece = GetSquare(move.GetTo());

        RemovePiece(move.GetTo(), cap_piece);
        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    case QUEEN_PROMOTION_CAPTURE:
    {
        const auto pawn_piece = Piece(PAWN, stm);
        const auto promo_piece = Piece(QUEEN, stm);
        const auto cap_piece = GetSquare(move.GetTo());

        RemovePiece(move.GetTo(), cap_piece);
        AddPiece(move.GetTo(), promo_piece);
        RemovePiece(move.GetFrom(), pawn_piece);

        key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());
        key ^= Zobrist::piece_square(promo_piece, move.GetTo());
        key ^= Zobrist::piece_square(cap_piece, move.GetTo());
        pawn_key ^= Zobrist::piece_square(pawn_piece, move.GetFrom());

        break;
    }
    default:
        assert(0);
    }

    if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, stm) || move.IsPromotion())
        fifty_move_count = 0;
    else
        fifty_move_count++;

    half_turn_count += 1;
    stm = !stm;

    assert(key == Zobrist::key(*this));
    assert(pawn_key == Zobrist::pawn_key(*this));
}

void BoardState::ApplyNullMove()
{
    key ^= Zobrist::stm();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
    {
        key ^= Zobrist::en_passant(GetFile(en_passant));
    }

    en_passant = N_SQUARES;
    fifty_move_count++;
    half_turn_count += 1;
    stm = !stm;

    assert(key == Zobrist::key(*this));
    assert(pawn_key == Zobrist::pawn_key(*this));
}

MoveFlag BoardState::GetMoveFlag(Square from, Square to) const
{
    // Castling (normal chess)
    if ((from == SQ_E1 && to == SQ_G1 && GetSquare(from) == WHITE_KING)
        || (from == SQ_E8 && to == SQ_G8 && GetSquare(from) == BLACK_KING))
    {
        return H_SIDE_CASTLE;
    }

    if ((from == SQ_E1 && to == SQ_C1 && GetSquare(from) == WHITE_KING)
        || (from == SQ_E8 && to == SQ_C8 && GetSquare(from) == BLACK_KING))
    {
        return A_SIDE_CASTLE;
    }

    // Castling (chess960). Needs to be handled before the 'capture' case
    if ((GetSquare(from) == WHITE_KING && GetSquare(to) == WHITE_ROOK)
        || (GetSquare(from) == BLACK_KING && GetSquare(to) == BLACK_ROOK))
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
    if (IsOccupied(to))
    {
        return CAPTURE;
    }

    // Double pawn moves
    if (AbsRankDiff(from, to) == 2 && (GetSquare(from) == WHITE_PAWN || GetSquare(from) == BLACK_PAWN))
    {
        return PAWN_DOUBLE_MOVE;
    }

    // En passant
    if (to == en_passant && (GetSquare(from) == WHITE_PAWN || GetSquare(from) == BLACK_PAWN))
    {
        return EN_PASSANT;
    }

    return QUIET;
}
