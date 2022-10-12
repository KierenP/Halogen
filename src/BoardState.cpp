#include "BoardState.h"

#include "Move.h"
#include "MoveGeneration.h"
#include "Network.h"
#include "Zobrist.h"

#include <iostream>

BoardState::BoardState()
{
    Reset();
}

void BoardState::Reset()
{
    en_passant = N_SQUARES;
    fifty_move_count = 0;
    turn_count = 1;

    stm = N_PLAYERS;
    white_king_castle = false;
    white_queen_castle = false;
    black_king_castle = false;
    black_queen_castle = false;

    board = {};

    RecalculateWhiteBlackBoards();
    key.Recalculate(*this);
}

bool BoardState::InitialiseFromFen(const std::vector<std::string>& fen)
{
    Reset();

    size_t FenLetter = 0; //index within the string
    int square = 0; //index within the board
    while ((square < 64) && (FenLetter < fen[0].length()))
    {
        char letter = fen[0].at(FenLetter);
        //ugly code, but surely this is the only time in the code that we iterate over squares in a different order than the Square enum
        //the squares values go up as you move up the board towards black's starting side, but a fen starts from the black side and works downwards
        Square sq = static_cast<Square>((RANK_8 - square / 8) * 8 + square % 8);

        // clang-format off
		switch (letter)
		{
		case 'p': SetSquare(sq, BLACK_PAWN); break;
		case 'r': SetSquare(sq, BLACK_ROOK); break;
		case 'n': SetSquare(sq, BLACK_KNIGHT); break;
		case 'b': SetSquare(sq, BLACK_BISHOP); break;
		case 'q': SetSquare(sq, BLACK_QUEEN); break;
		case 'k': SetSquare(sq, BLACK_KING); break;
		case 'P': SetSquare(sq, WHITE_PAWN); break;
		case 'R': SetSquare(sq, WHITE_ROOK); break;
		case 'N': SetSquare(sq, WHITE_KNIGHT); break;
		case 'B': SetSquare(sq, WHITE_BISHOP); break;
		case 'Q': SetSquare(sq, WHITE_QUEEN); break;
		case 'K': SetSquare(sq, WHITE_KING); break;
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

    if (fen[1] == "b")
        stm = BLACK;

    if (fen[2].find('K') != std::string::npos)
        white_king_castle = true;

    if (fen[2].find('Q') != std::string::npos)
        white_queen_castle = true;

    if (fen[2].find('k') != std::string::npos)
        black_king_castle = true;

    if (fen[2].find('q') != std::string::npos)
        black_queen_castle = true;

    en_passant = static_cast<Square>(AlgebraicToPos(fen[3]));

    fifty_move_count = std::stoi(fen[4]);
    turn_count = std::stoi(fen[5]);

    RecalculateWhiteBlackBoards();
    key.Recalculate(*this);
    return true;
}

void BoardState::RecalculateWhiteBlackBoards()
{
    WhitePieces = GetPieceBB(WHITE_PAWN) | GetPieceBB(WHITE_KNIGHT) | GetPieceBB(WHITE_BISHOP) | GetPieceBB(WHITE_ROOK) | GetPieceBB(WHITE_QUEEN) | GetPieceBB(WHITE_KING);
    BlackPieces = GetPieceBB(BLACK_PAWN) | GetPieceBB(BLACK_KNIGHT) | GetPieceBB(BLACK_BISHOP) | GetPieceBB(BLACK_ROOK) | GetPieceBB(BLACK_QUEEN) | GetPieceBB(BLACK_KING);
}

uint64_t BoardState::GetPiecesColour(Players colour) const
{
    if (colour == WHITE)
        return GetWhitePieces();
    else
        return GetBlackPieces();
}

uint64_t BoardState::GetZobristKey() const
{
    return key.Key();
}

void BoardState::SetSquare(Square square, Pieces piece)
{
    assert(square < N_SQUARES);
    assert(piece < N_PIECES);

    ClearSquare(square);

    if (piece < N_PIECES) //it is possible we might set a square to be empty using this function rather than using the ClearSquare function below.
        board[piece] |= SquareBB[square];

    RecalculateWhiteBlackBoards();
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

    RecalculateWhiteBlackBoards();
}

uint64_t BoardState::GetPieceBB(PieceTypes pieceType, Players colour) const
{
    return GetPieceBB(Piece(pieceType, colour));
}

Square BoardState::GetKing(Players colour) const
{
    assert(GetPieceBB(KING, colour) != 0); //assert only runs in debug so I don't care about the double call
    return static_cast<Square>(LSB(GetPieceBB(KING, colour)));
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

bool BoardState::IsOccupied(Square square, Players colour) const
{
    assert(square != N_SQUARES);
    return colour == WHITE ? (GetWhitePieces() & SquareBB[square]) : (GetBlackPieces() & SquareBB[square]);
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
    return GetWhitePieces() | GetBlackPieces();
}

uint64_t BoardState::GetEmptySquares() const
{
    return ~GetAllPieces();
}

uint64_t BoardState::GetWhitePieces() const
{
    return WhitePieces;
}

uint64_t BoardState::GetBlackPieces() const
{
    return BlackPieces;
}

void BoardState::UpdateCastleRights(Move move, Zobrist& zobrist)
{
    if (move.GetFrom() == SQ_E1 || move.GetTo() == SQ_E1) //Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
    {
        if (white_king_castle)
            zobrist.ToggleWhiteKingside();

        if (white_queen_castle)
            zobrist.ToggleWhiteQueenside();

        white_king_castle = false;
        white_queen_castle = false;
    }

    if (move.GetFrom() == SQ_E8 || move.GetTo() == SQ_E8)
    {
        if (black_king_castle)
            zobrist.ToggleBlackKingsize();

        if (black_queen_castle)
            zobrist.ToggleBlackQueensize();

        black_king_castle = false;
        black_queen_castle = false;
    }

    if (move.GetFrom() == SQ_A1 || move.GetTo() == SQ_A1)
    {
        if (white_queen_castle)
            zobrist.ToggleWhiteQueenside();

        white_queen_castle = false;
    }

    if (move.GetFrom() == SQ_A8 || move.GetTo() == SQ_A8)
    {
        if (black_queen_castle)
            zobrist.ToggleBlackQueensize();

        black_queen_castle = false;
    }

    if (move.GetFrom() == SQ_H1 || move.GetTo() == SQ_H1)
    {
        if (white_king_castle)
            zobrist.ToggleWhiteKingside();

        white_king_castle = false;
    }

    if (move.GetFrom() == SQ_H8 || move.GetTo() == SQ_H8)
    {
        if (black_king_castle)
            zobrist.ToggleBlackKingsize();

        black_king_castle = false;
    }
}

void BoardState::Print() const
{
    std::cout << "\n  A B C D E F G H";

    char Letter[N_SQUARES];

    for (int i = 0; i < N_SQUARES; i++)
    {
        Letter[i] = PieceToChar(GetSquare(static_cast<Square>(i)));
    }

    for (int i = 0; i < N_SQUARES; i++)
    {
        unsigned int square = GetPosition(GetFile(i), 7 - GetRank(i)); //7- to flip on the y axis and do rank 8, 7 ...

        if (GetFile(square) == FILE_A)
        {
            std::cout << std::endl; //Go to a new line
            std::cout << 8 - GetRank(i); //Count down from 8
        }

        std::cout << " ";
        std::cout << Letter[square];
    }

    std::cout << std::endl;
}

void BoardState::ApplyMove(Move move, Network& net)
{
    key.ToggleSTM();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
        key.ToggleEnpassant(GetFile(en_passant));

    en_passant = N_SQUARES;
    fifty_move_count++;

    if (move.IsCapture() && move.GetFlag() != EN_PASSANT)
    {
        net.RemoveInput(move.GetTo(), GetSquare(move.GetTo()));
        key.TogglePieceSquare(GetSquare(move.GetTo()), move.GetTo());
    }

    if (!move.IsPromotion())
        SetSquareAndUpdate(move.GetTo(), GetSquare(move.GetFrom()), net);

    if (move.GetFlag() == KING_CASTLE)
    {
        SetSquareAndUpdate(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))), net);
        ClearSquareAndUpdate(GetPosition(FILE_H, GetRank(move.GetFrom())), net);
    }

    if (move.GetFlag() == QUEEN_CASTLE)
    {
        SetSquareAndUpdate(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))), net);
        ClearSquareAndUpdate(GetPosition(FILE_A, GetRank(move.GetFrom())), net);
    }

    //for some special moves we need to do other things
    switch (move.GetFlag())
    {
    case PAWN_DOUBLE_MOVE:
    {
        //average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white
        Square ep = static_cast<Square>((move.GetTo() + move.GetFrom()) / 2);

        //it only counts as a ep square if a pawn could potentially do the capture!
        uint64_t potentialAttackers = PawnAttacks[stm][ep] & GetPieceBB(PAWN, !stm);

        while (potentialAttackers)
        {
            Square sq = static_cast<Square>(LSBpop(potentialAttackers));

            // NOTE: would make/unmake be faster than copy/delete?
            BoardState copy = *this;
            Move ep_move(sq, ep, EN_PASSANT);
            copy.SetSquare(ep_move.GetTo(), copy.GetSquare(ep_move.GetFrom()));
            copy.ClearSquare(ep_move.GetFrom());
            copy.ClearSquare(GetPosition(GetFile(ep_move.GetTo()), GetRank(ep_move.GetFrom())));
            bool legal = !IsInCheck(copy, !stm);

            if (legal)
            {
                en_passant = ep;
                key.ToggleEnpassant(GetFile(ep));
                break;
            }
        }

        break;
    }
    case EN_PASSANT:
        ClearSquareAndUpdate(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())), net);
        break;
    case KNIGHT_PROMOTION:
    case KNIGHT_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(KNIGHT, stm), net);
        break;
    case BISHOP_PROMOTION:
    case BISHOP_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(BISHOP, stm), net);
        break;
    case ROOK_PROMOTION:
    case ROOK_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(ROOK, stm), net);
        break;
    case QUEEN_PROMOTION:
    case QUEEN_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(QUEEN, stm), net);
        break;
    default:
        break;
    }

    if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, stm) || move.IsPromotion())
        fifty_move_count = 0;

    ClearSquareAndUpdate(move.GetFrom(), net);
    turn_count += 1;
    stm = !stm;
    UpdateCastleRights(move, key);

    assert(key.Verify(*this));
}

void BoardState::ApplyNullMove()
{
    key.ToggleSTM();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
        key.ToggleEnpassant(GetFile(en_passant));

    en_passant = N_SQUARES;
    fifty_move_count++;
    turn_count += 1;
    stm = !stm;

    assert(key.Verify(*this));
}

void BoardState::SetSquareAndUpdate(Square square, Pieces piece, Network& net)
{
    net.AddInput(square, piece);
    key.TogglePieceSquare(piece, square);
    SetSquare(square, piece);
}

void BoardState::ClearSquareAndUpdate(Square square, Network& net)
{
    Pieces piece = GetSquare(square);
    net.RemoveInput(square, piece);
    key.TogglePieceSquare(piece, square);
    ClearSquare(square);
}