#include "Position.h"

std::vector<uint64_t> PreviousKeys;

Position::Position()
{
	
}

Position::Position(std::vector<std::string> moves)
{
	InitialiseFromMoves(moves);
}

Position::Position(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount)
{
	InitialiseFromFen(board, turn, castle, ep, fiftyMove, turnCount);
}

Position::Position(std::string fen)
{
	InitialiseFromFen(fen);
}


Position::~Position()
{
}

void Position::ApplyMove(Move move)
{
	/*SaveParamiters();

	SetEnPassant(-1);
	SetCaptureSquare(-1);
	SetCapturePiece(-1);

	if (move.IsPromotion())
		SetPromotionPiece(GetSquare(move.GetFrom()));

	if (move.IsCastle())
		GetTurn() == WHITE ? WhiteCastled() : BlackCastled();

	if (move.IsCapture() && move.GetFlag() != EN_PASSANT)
	{
		SetCapturePiece(GetSquare(move.GetTo()));
		SetCaptureSquare(move.GetTo());
	}

	switch (move.GetFlag())
	{
	case QUIET:
		break;
	case PAWN_DOUBLE_MOVE:
		SetEnPassant((move.GetTo() + move.GetFrom()) / 2);																	//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white 
		break;
	case KING_CASTLE:
		SetSquare(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))));	//move the rook
		ClearSquare(GetPosition(FILE_H, GetRank(move.GetFrom())));	
		break;
	case QUEEN_CASTLE:
		SetSquare(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_A, GetRank(move.GetFrom())));
		break;
	case CAPTURE:
		break;
	case EN_PASSANT:
		SetCapturePiece(GetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))));
		SetCaptureSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		break;
	case NULL_MOVE:
		break;
	case KNIGHT_PROMOTION:
		SetSquare(move.GetTo(), Piece(KNIGHT, GetTurn()));
		break;
	case BISHOP_PROMOTION:
		SetSquare(move.GetTo(), Piece(BISHOP, GetTurn()));
		break;
	case ROOK_PROMOTION:
		SetSquare(move.GetTo(), Piece(ROOK, GetTurn()));
		break;
	case QUEEN_PROMOTION:
		SetSquare(move.GetTo(), Piece(QUEEN, GetTurn()));
		break;
	case KNIGHT_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), Piece(KNIGHT, GetTurn()));
		break;
	case BISHOP_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), Piece(BISHOP, GetTurn()));
		break;
	case ROOK_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), Piece(ROOK, GetTurn()));
		break;
	case QUEEN_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), Piece(QUEEN, GetTurn()));
		break;
	default:
		break;
	}

	if (move.GetFlag() != NULL_MOVE && !move.IsPromotion())
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));

	if (move.GetFlag() != NULL_MOVE)
		ClearSquare(move.GetFrom());

	NextTurn();
	UpdateCastleRights(move);
	//GenerateAttackTables();*/

	//PositionParam* oldparam = new PositionParam(BoardParamiter);		//make a copy of the old
	//BoardParamiter.Prev = oldparam;										//set current's previous to that copy
	SaveParamiters();
	SetEnPassant(-1);
	SetCaptureSquare(-1);

	switch (move.GetFlag())
	{
	case QUIET:
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case PAWN_DOUBLE_MOVE:
		SetEnPassant((move.GetTo() + move.GetFrom()) / 2);			//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white 
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case KING_CASTLE:
		if (GetTurn() == WHITE)
			WhiteCastled();
		else
			BlackCastled();
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		SetSquare(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_H, GetRank(move.GetFrom())));
		break;
	case QUEEN_CASTLE:
		if (GetTurn() == WHITE)
			WhiteCastled();
		else
			BlackCastled();
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		SetSquare(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_A, GetRank(move.GetFrom())));
		break;
	case CAPTURE:
		SetCapturePiece(GetSquare(move.GetTo()));
		SetCaptureSquare(move.GetTo());
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case EN_PASSANT:
		SetCapturePiece(GetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))));
		SetCaptureSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		break;
	case KNIGHT_PROMOTION:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetSquare(move.GetTo(), Piece(KNIGHT, GetTurn()));
		break;
	case BISHOP_PROMOTION:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetSquare(move.GetTo(), Piece(BISHOP, GetTurn()));
		break;
	case ROOK_PROMOTION:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetSquare(move.GetTo(), Piece(ROOK, GetTurn()));
		break;
	case QUEEN_PROMOTION:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetSquare(move.GetTo(), Piece(QUEEN, GetTurn()));
		break;
	case KNIGHT_PROMOTION_CAPTURE:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetCapturePiece(GetSquare(move.GetTo()));
		SetSquare(move.GetTo(), Piece(KNIGHT, GetTurn()));
		break;
	case BISHOP_PROMOTION_CAPTURE:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetCapturePiece(GetSquare(move.GetTo()));
		SetSquare(move.GetTo(), Piece(BISHOP, GetTurn()));
		break;
	case ROOK_PROMOTION_CAPTURE:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetCapturePiece(GetSquare(move.GetTo()));
		SetSquare(move.GetTo(), Piece(ROOK, GetTurn()));
		break;
	case QUEEN_PROMOTION_CAPTURE:
		SetPromotionPiece(GetSquare(move.GetFrom()));
		SetCapturePiece(GetSquare(move.GetTo()));
		SetSquare(move.GetTo(), Piece(QUEEN, GetTurn()));
		break;
	default:
		break;
	}

	ClearSquare(move.GetFrom());
	NextTurn();
	UpdateCastleRights(move);
	PreviousKeys.push_back(GenerateZobristKey(*this));
	GenerateAttackTables();
}

void Position::ApplyMove(std::string strmove)
{
	unsigned int prev = (strmove[0] - 97) + (strmove[1] - 49) * 8;
	unsigned int next = (strmove[2] - 97) + (strmove[3] - 49) * 8;
	unsigned int flag = QUIET;

	//Captures
	if (IsOccupied(next))
		flag = CAPTURE;

	//Double pawn moves
	if (AbsRankDiff(prev, next) == 2)
		if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
			flag = PAWN_DOUBLE_MOVE;

	//En passant
	if (next == GetEnPassant())
		if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
			flag = EN_PASSANT;

	//Castling
	if (prev == SQ_E1 && next == SQ_G1 && GetSquare(prev) == WHITE_KING)
		flag = KING_CASTLE;

	if (prev == SQ_E1 && next == SQ_C1 && GetSquare(prev) == WHITE_KING)
		flag = QUEEN_CASTLE;

	if (prev == SQ_E8 && next == SQ_G8 && GetSquare(prev) == BLACK_KING)
		flag = KING_CASTLE;

	if (prev == SQ_E8 && next == SQ_C8 && GetSquare(prev) == BLACK_KING)
		flag = QUEEN_CASTLE;

	//Promotion
	if (strmove.length() == 5)		//promotion: c7c8q or d2d1n	etc.
	{
		if (tolower(strmove[4]) == 'n')
			flag = KNIGHT_PROMOTION;
		if (tolower(strmove[4]) == 'r')
			flag = ROOK_PROMOTION;
		if (tolower(strmove[4]) == 'q')
			flag = QUEEN_PROMOTION;
		if (tolower(strmove[4]) == 'b')
			flag = BISHOP_PROMOTION;

		if (flag == CAPTURE)	//EN_PASSANT flag with promotion is impossible, if its a capture we need to shift the flag up 4 to turn it from eg KNIGHT_PROMOTION to KNIGHT_PROMOTION_CAPTURE
			flag += 4;
	}

	ApplyMove(Move(prev, next, flag));
}

void Position::RevertMove(Move move)
{
	SetSquare(move.GetFrom(), GetSquare(move.GetTo()));
	ClearSquare(move.GetTo());

	switch (move.GetFlag())
	{
	case QUIET:
		break;
	case PAWN_DOUBLE_MOVE:
		break;
	case KING_CASTLE:
		SetSquare(GetPosition(FILE_H, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))));		//move the rook back
		ClearSquare(GetPosition(FILE_F, GetRank(move.GetFrom())));
		break;
	case QUEEN_CASTLE:
		SetSquare(GetPosition(FILE_A, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))));		//move the rook back
		ClearSquare(GetPosition(FILE_D, GetRank(move.GetFrom())));
		break;
	case CAPTURE:
		SetSquare(move.GetTo(), GetCapturePiece());
		break;
	case EN_PASSANT:
		SetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())), GetCapturePiece());
		break;
	case KNIGHT_PROMOTION:
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case BISHOP_PROMOTION:
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case ROOK_PROMOTION:
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case QUEEN_PROMOTION:
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case KNIGHT_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), GetCapturePiece());
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case BISHOP_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), GetCapturePiece());
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case ROOK_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), GetCapturePiece());
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	case QUEEN_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), GetCapturePiece());
		SetSquare(move.GetFrom(), GetPromotionPiece());
		break;
	default:
		break;
	}

	RestorePreviousParamiters();
	PreviousKeys.pop_back();
	GenerateAttackTables();
}

void Position::ApplyNullMove()
{
	SaveParamiters();
	SetEnPassant(-1);
	SetCaptureSquare(-1);

	NextTurn();
	PreviousKeys.push_back(GenerateZobristKey(*this));
}

void Position::RevertNullMove()
{
	RestorePreviousParamiters();
	PreviousKeys.pop_back();
}

void Position::Print() const
{
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hConsole, 10);	//green text
	std::cout << "\n  A B C D E F G H";
	SetConsoleTextAttribute(hConsole, 7);	//back to gray

	char Letter[N_SQUARES];
	unsigned int colour[N_SQUARES];

	for (int i = 0; i < N_SQUARES; i++)
	{
		Letter[i] = PieceToChar(GetSquare(i));
		colour[i] = 7;									//grey
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		unsigned int square = GetPosition(GetFile(i), 7 - GetRank(i));		//7- to flip on the y axis and do rank 8, 7 ... 

		if (GetFile(square) == FILE_A)
		{
			std::cout << std::endl;									//Go to a new line
			SetConsoleTextAttribute(hConsole, 10);					//print the number green
			std::cout << 8 - GetRank(i);							//Count down from 8
		}

		std::cout << " ";

		SetConsoleTextAttribute(hConsole, colour[square]);			//Set colour to that squares colour
		std::cout << Letter[square];
	}

	SetConsoleTextAttribute(hConsole, 7);							//and back to gray

	std::cout << std::endl;
}

void Position::StartingPosition()
{
	InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
}

bool Position::InitialiseFromFen(std::vector<std::string> fen)
{
	if (fen.size() != 6)
		return false;							//bad fen

	InitialiseBoardFromFen(fen);
	InitialiseParamitersFromFen(fen);

	return true;
}

bool Position::InitialiseFromFen(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount)
{
	std::vector<std::string> splitFen;		
	splitFen.push_back(board);
	splitFen.push_back(turn);
	splitFen.push_back(castle);
	splitFen.push_back(ep);
	splitFen.push_back(fiftyMove);
	splitFen.push_back(turnCount);
	return InitialiseFromFen(splitFen);
}

bool Position::InitialiseFromFen(std::string fen)
{
	std::vector<std::string> splitFen;			//Split the line into an array of strings seperated by each space
	std::istringstream iss(fen);
	splitFen.push_back("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
	splitFen.push_back("w");
	splitFen.push_back("-");
	splitFen.push_back("-");
	splitFen.push_back("0");
	splitFen.push_back("1");

	for (int i = 0; i < 6 && iss; i++)
	{
		std::string stub;
		iss >> stub;
		if (stub != "")
			splitFen[i] = (stub);
	}

	return InitialiseFromFen(splitFen[0], splitFen[1], splitFen[2], splitFen[3], splitFen[4], splitFen[5]);
}

bool Position::InitialiseFromMoves(std::vector<std::string> moves)
{
	StartingPosition();

	for (int i = 0; i < moves.size(); i++)
	{
		ApplyMove(moves[i]);
	}

	return true;
}
