#include "BoardState.h"

#include "BitBoardDefine.h"
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
    half_turn_count = 1;

    stm = N_PLAYERS;
    castle_squares = EMPTY;

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
            castle_squares |= SquareBB[MSB(GetPieceBB(WHITE_ROOK) & RankBB[RANK_1])];

        else if (letter == 'Q')
            castle_squares |= SquareBB[LSB(GetPieceBB(WHITE_ROOK) & RankBB[RANK_1])];

        else if (letter == 'k')
            castle_squares |= SquareBB[MSB(GetPieceBB(BLACK_ROOK) & RankBB[RANK_8])];

        else if (letter == 'q')
            castle_squares |= SquareBB[LSB(GetPieceBB(BLACK_ROOK) & RankBB[RANK_8])];

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

    fifty_move_count = std::stoi(fen[4]);
    half_turn_count = std::stoi(fen[5]) * 2 - (stm == WHITE); // convert full move count to half move count

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
    // check for the king or rook moving, or a rook being captured
    // we must remember that the move has already been applied

    if (move.GetTo() == GetKing(WHITE))
    {
        // get castle squares on white side
        uint64_t white_castle = castle_squares & RankBB[RANK_1];

        while (white_castle)
        {
            auto sq = LSBpop(white_castle);
            zobrist.ToggleCastle(sq);
        }

        // switch any set bits on first rank to 0
        castle_squares &= ~(RankBB[RANK_1]);
    }

    if (move.GetTo() == GetKing(BLACK))
    {
        // get castle squares on black side
        uint64_t black_castle = castle_squares & RankBB[RANK_8];

        while (black_castle)
        {
            auto sq = LSBpop(black_castle);
            zobrist.ToggleCastle(sq);
        }

        // switch any set bits on first rank to 0
        castle_squares &= ~(RankBB[RANK_8]);
    }

    if (SquareBB[move.GetTo()] & castle_squares)
    {
        zobrist.ToggleCastle(move.GetTo());
        castle_squares &= ~SquareBB[move.GetTo()];
    }

    if (SquareBB[move.GetFrom()] & castle_squares)
    {
        zobrist.ToggleCastle(move.GetFrom());
        castle_squares &= ~SquareBB[move.GetFrom()];
    }
}

void BoardState::Print() const
{
    std::cout << "\n  A B C D E F G H";

    char Letter[N_SQUARES];

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        constexpr static char PieceChar[13] = { 'p', 'n', 'b', 'r', 'q', 'k', 'P', 'N', 'B', 'R', 'Q', 'K', ' ' };
        Letter[i] = PieceChar[GetSquare(i)];
    }

    for (Square i = SQ_A1; i <= SQ_H8; ++i)
    {
        auto square = GetPosition(GetFile(i), Mirror(GetRank(i)));

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

    switch (move.GetFlag())
    {
    case QUIET:
        SetSquareAndUpdate(move.GetTo(), GetSquare(move.GetFrom()), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case PAWN_DOUBLE_MOVE:
    {
        //average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white
        Square ep_sq = static_cast<Square>((move.GetTo() + move.GetFrom()) / 2);

        //it only counts as a ep square if a pawn could potentially do the capture!
        uint64_t potentialAttackers = PawnAttacks[stm][ep_sq] & GetPieceBB(PAWN, !stm);

        while (potentialAttackers)
        {
            Square threat_sq = LSBpop(potentialAttackers);

            // carefully apply the potential ep capture, check for legality, then undo
            SetSquare(ep_sq, Piece(PAWN, !stm));
            ClearSquare(threat_sq);
            ClearSquare(move.GetTo());
            bool legal = !IsInCheck(*this, !stm);
            ClearSquare(ep_sq);
            SetSquare(threat_sq, Piece(PAWN, !stm));
            SetSquare(move.GetTo(), Piece(PAWN, stm));

            if (legal)
            {
                en_passant = ep_sq;
                key.ToggleEnpassant(GetFile(ep_sq));
                break;
            }
        }

        SetSquareAndUpdate(move.GetTo(), Piece(PAWN, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    }
    case A_SIDE_CASTLE:
    {
        Square king_start = move.GetFrom();
        Square king_end = move.GetTo();
        Square rook_start = LSB(castle_squares & RankBB[stm == WHITE ? RANK_1 : RANK_8]);
        Square rook_end = stm == WHITE ? SQ_D1 : SQ_D8;

        ClearSquareAndUpdate(king_start, net);
        ClearSquareAndUpdate(rook_start, net);
        SetSquareAndUpdate(king_end, Piece(KING, stm), net);
        SetSquareAndUpdate(rook_end, Piece(ROOK, stm), net);

        break;
    }
    case H_SIDE_CASTLE:
    {
        Square king_start = move.GetFrom();
        Square king_end = move.GetTo();
        Square rook_start = MSB(castle_squares & RankBB[stm == WHITE ? RANK_1 : RANK_8]);
        Square rook_end = stm == WHITE ? SQ_F1 : SQ_F8;

        ClearSquareAndUpdate(king_start, net);
        ClearSquareAndUpdate(rook_start, net);
        SetSquareAndUpdate(king_end, Piece(KING, stm), net);
        SetSquareAndUpdate(rook_end, Piece(ROOK, stm), net);

        break;
    }
    case CAPTURE:
        ClearSquareAndUpdate(move.GetTo(), net);
        SetSquareAndUpdate(move.GetTo(), GetSquare(move.GetFrom()), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case EN_PASSANT:
        SetSquareAndUpdate(move.GetTo(), GetSquare(move.GetFrom()), net);
        ClearSquareAndUpdate(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case KNIGHT_PROMOTION:
        SetSquareAndUpdate(move.GetTo(), Piece(KNIGHT, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case BISHOP_PROMOTION:
        SetSquareAndUpdate(move.GetTo(), Piece(BISHOP, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case ROOK_PROMOTION:
        SetSquareAndUpdate(move.GetTo(), Piece(ROOK, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case QUEEN_PROMOTION:
        SetSquareAndUpdate(move.GetTo(), Piece(QUEEN, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case KNIGHT_PROMOTION_CAPTURE:
        ClearSquareAndUpdate(move.GetTo(), net);
        SetSquareAndUpdate(move.GetTo(), Piece(KNIGHT, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case BISHOP_PROMOTION_CAPTURE:
        ClearSquareAndUpdate(move.GetTo(), net);
        SetSquareAndUpdate(move.GetTo(), Piece(BISHOP, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case ROOK_PROMOTION_CAPTURE:
        ClearSquareAndUpdate(move.GetTo(), net);
        SetSquareAndUpdate(move.GetTo(), Piece(ROOK, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    case QUEEN_PROMOTION_CAPTURE:
        ClearSquareAndUpdate(move.GetTo(), net);
        SetSquareAndUpdate(move.GetTo(), Piece(QUEEN, stm), net);
        ClearSquareAndUpdate(move.GetFrom(), net);
        break;
    default:
        assert(0);
    }

    UpdateCastleRights(move, key);

    if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, stm) || move.IsPromotion())
        fifty_move_count = 0;
    else
        fifty_move_count++;

    half_turn_count += 1;
    stm = !stm;

    assert(key.Verify(*this));
    assert(net.Verify(*this));
}

void BoardState::ApplyNullMove()
{
    key.ToggleSTM();

    // undo the previous ep square
    if (en_passant <= SQ_H8)
        key.ToggleEnpassant(GetFile(en_passant));

    en_passant = N_SQUARES;
    fifty_move_count++;
    half_turn_count += 1;
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