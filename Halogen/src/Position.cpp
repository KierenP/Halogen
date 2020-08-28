#include "Position.h"

Position::Position()
{
	key = EMPTY;
	NodeCount = 0;
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
	PreviousKeys.push_back(key);
	SaveParamiters();
	SaveBoard();
	SetEnPassant(static_cast<unsigned int>(-1));
	Increment50Move();
	NodeCount += 1;

	SetSquare(move.GetTo(), GetSquare(move.GetFrom()));

	if (move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE)
	{
		if (GetTurn() == WHITE)
			WhiteCastled();
		else
			BlackCastled();
	}

	if (move.GetFlag() == KING_CASTLE)
	{
		SetSquare(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_H, GetRank(move.GetFrom())));
	}

	if (move.GetFlag() == QUEEN_CASTLE)
	{
		SetSquare(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_A, GetRank(move.GetFrom())));
	}

	//for some special moves we need to do other things
	switch (move.GetFlag())
	{
	case PAWN_DOUBLE_MOVE:
		SetEnPassant((move.GetTo() + move.GetFrom()) / 2);			//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white 
		break;
	case EN_PASSANT:
		ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
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

	if (move.IsCapture())
		SetCaptureSquare(move.GetTo());
	else
		SetCaptureSquare(static_cast<unsigned int>(-1));

	if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, GetTurn()) || move.IsPromotion())
		Reset50Move();

	ClearSquare(move.GetFrom());
	NextTurn();
	UpdateCastleRights(move);
	IncrementZobristKey(move);

	/*if (GenerateZobristKey() != key)
	{
		std::cout << "error";
		NextTurn();	//just adding something here to really mess things up
	}*/
	
	//In order to see if the key was corrupted at some point
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

		if (IsOccupied(next))				//if it's a capture we need to shift the flag up 4 to turn it from eg: KNIGHT_PROMOTION to KNIGHT_PROMOTION_CAPTURE. EDIT: flag = capture wont work because we just changed the flag!! This was a bug back from 2018 found in 2020
			flag += 4;
	}

	ApplyMove(Move(prev, next, flag));
}

void Position::RevertMove()
{
	assert(PreviousKeys.size() > 0);

	RestorePreviousBoard();
	RestorePreviousParamiters();
	key = PreviousKeys.back();
	PreviousKeys.pop_back();
}

void Position::ApplyNullMove()
{
	PreviousKeys.push_back(key);
	SaveParamiters();
	SetEnPassant(static_cast<unsigned int>(-1));
	SetCaptureSquare(static_cast<unsigned int>(-1));
	Increment50Move();

	NextTurn();
	IncrementZobristKey(Move());

	/*if (GenerateZobristKey() != key)
	{
		std::cout << "error";
		NextTurn();
	}*/
}

void Position::RevertNullMove()
{
	assert(PreviousKeys.size() > 0);

	RestorePreviousParamiters();
	key = PreviousKeys.back();
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
	unsigned short colour[N_SQUARES];

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

	if (!InitialiseBoardFromFen(fen))
		return false;

	if (!InitialiseParamitersFromFen(fen))
		return false;

	key = GenerateZobristKey();

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

	for (size_t i = 0; i < moves.size(); i++)
	{
		ApplyMove(moves[i]);
	}

	return true;
}

uint64_t Position::GetZobristKey() const
{
	return key;
}

void Position::Reset()
{
	PreviousKeys.clear();
	key = EMPTY;
	NodeCount = 0;

	ResetBoard();
	InitParamiters();
}

uint64_t Position::GetPreviousKey(size_t index)
{
	assert(index < PreviousKeys.size());
	return PreviousKeys.at(index);
}

void Position::FlipColours()
{
	ApplyNullMove();
	SwapCastelingRights();

	for (int i = 0; i < N_SQUARES; i++)
	{
		if (IsOccupied(i))
			SetSquare(i, Piece(GetSquare(i) % N_PIECE_TYPES, !ColourOfPiece(GetSquare(i))));
	}
}

void Position::MirrorLeftRight()
{
	Position copy = *this;

	for (int i = 0; i < N_SQUARES; i++)
	{
		ClearSquare(i);
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		if (copy.IsOccupied(GetPosition(N_FILES - GetFile(i) - 1, GetRank(i))))
			SetSquare(i, copy.GetSquare(GetPosition(N_FILES - GetFile(i) - 1, GetRank(i))));
	}
}

void Position::MirrorTopBottom()
{
	Position copy = *this;

	for (int i = 0; i < N_SQUARES; i++)
	{
		ClearSquare(i);
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		if (copy.IsOccupied(GetPosition(GetFile(i), N_RANKS - GetRank(i) - 1)))
			SetSquare(i, copy.GetSquare(GetPosition(GetFile(i), N_RANKS - GetRank(i) - 1)));
	}
}

uint64_t Position::GenerateZobristKey() const
{
	uint64_t Key = EMPTY;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bitboard = GetPieceBB(i);
		while (bitboard != 0)
		{
			Key ^= ZobristTable.at(i * 64 + bitScanForwardErase(bitboard));
		}
	}

	if (GetTurn() == WHITE)
		Key ^= ZobristTable.at(12 * 64);

	if (CanCastleWhiteKingside())
		Key ^= ZobristTable.at(12 * 64 + 1);
	if (CanCastleWhiteQueenside())
		Key ^= ZobristTable.at(12 * 64 + 2);
	if (CanCastleBlackKingside())
		Key ^= ZobristTable.at(12 * 64 + 3);
	if (CanCastleBlackQueenside())
		Key ^= ZobristTable.at(12 * 64 + 4);

	if (GetEnPassant() <= SQ_H8)														//no ep = -1 which wraps around to a very large number
	{
		Key ^= ZobristTable.at(12 * 64 + 5 + GetFile(GetEnPassant()));
	}

	return Key;
}

uint64_t Position::IncrementZobristKey(Move move) 
{
	const BoardParamiterData prev = GetPreviousParamiters();

	//Change of turn
	key ^= ZobristTable[12 * 64];	//because who's turn it is changed

	if (prev.GetEnPassant() <= SQ_H8)
		key ^= ZobristTable[(12 * 64 + 5 + GetFile(prev.GetEnPassant()))];		//undo the previous ep square

	if (move.IsUninitialized()) return key;	//null move

	if (!move.IsPromotion())
	{
		key ^= ZobristTable[GetSquare(move.GetTo()) * 64 + move.GetFrom()];	//toggle the square we left
		key ^= ZobristTable[GetSquare(move.GetTo()) * 64 + move.GetTo()];	//toggle the arriving square
	}

	//En Passant
	if (move.GetFlag() == EN_PASSANT)
		key ^= ZobristTable[Piece(PAWN, GetTurn()) * 64 + GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];	//remove the captured piece

	if (GetEnPassant() <= SQ_H8)
		key ^= ZobristTable[12 * 64 + 5 + GetFile(GetEnPassant())];									//apply new EP

	//Captures
	if ((move.IsCapture()) && (move.GetFlag() != EN_PASSANT))
		key ^= ZobristTable[GetPreviousBoard().GetSquare(move.GetTo()) * 64 + move.GetTo()];

	//Castling
	if (CanCastleWhiteKingside() != prev.CanCastleWhiteKingside())					//if casteling rights changed, flip that one
		key ^= ZobristTable[12 * 64 + 1];
	if (CanCastleWhiteQueenside() != prev.CanCastleWhiteQueenside())
		key ^= ZobristTable[12 * 64 + 2];
	if (CanCastleBlackKingside() != prev.CanCastleBlackKingside())
		key ^= ZobristTable[12 * 64 + 3];
	if (CanCastleBlackQueenside() != prev.CanCastleBlackQueenside())
		key ^= ZobristTable[12 * 64 + 4];

	if (move.GetFlag() == KING_CASTLE)
	{
		key ^= ZobristTable[GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_H, GetRank(move.GetFrom()))];
		key ^= ZobristTable[GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_F, GetRank(move.GetFrom()))];
	}

	if (move.GetFlag() == QUEEN_CASTLE)
	{
		key ^= ZobristTable[GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_A, GetRank(move.GetFrom()))];
		key ^= ZobristTable[GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_D, GetRank(move.GetFrom()))];
	}

	//Promotions
	if (move.IsPromotion())
	{
		key ^= ZobristTable[Piece(PAWN, !GetTurn()) * 64 + move.GetFrom()];

		if (move.GetFlag() == KNIGHT_PROMOTION || move.GetFlag() == KNIGHT_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(KNIGHT, !GetTurn()) * 64 + move.GetTo()];
		if (move.GetFlag() == BISHOP_PROMOTION || move.GetFlag() == BISHOP_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(BISHOP, !GetTurn()) * 64 + move.GetTo()];
		if (move.GetFlag() == ROOK_PROMOTION || move.GetFlag() == ROOK_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(ROOK, !GetTurn()) * 64 + move.GetTo()];
		if (move.GetFlag() == QUEEN_PROMOTION || move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(QUEEN, !GetTurn()) * 64 + move.GetTo()];
	}

	return key;
}

void Position::ApplySEECapture(Move move)
{
	SaveBoard();

	SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
	ClearSquare(move.GetFrom());
}

void Position::RevertSEECapture()
{
	RestorePreviousBoard();
}
