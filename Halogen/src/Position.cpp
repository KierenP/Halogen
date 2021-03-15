#include "Position.h"

Position::Position()
{
	StartingPosition();
}

void Position::ApplyMove(Move move)
{
	PreviousKeys.push_back(key);
	SaveParameters();
	SaveBoard();
	SetEnPassant(N_SQUARES);
	Increment50Move();

	if (IsCapture(move))
	{
		SetCaptureSquare(GetTo(move));
		SetCapturePiece(GetSquare(GetTo(move)));	//will fail with en passant, but that currently doesn't matter
	}
	else
	{
		SetCaptureSquare(N_SQUARES);
		SetCapturePiece(N_PIECES);
	}

	SetSquare(GetTo(move), GetSquare(GetFrom(move)));

	if (GetFlag(move) == KING_CASTLE || GetFlag(move) == QUEEN_CASTLE)
	{
		if (GetTurn() == WHITE)
			WhiteCastled();
		else
			BlackCastled();
	}

	if (GetFlag(move) == KING_CASTLE)
	{
		SetSquare(GetPosition(FILE_F, GetRank(GetFrom(move))), GetSquare(GetPosition(FILE_H, GetRank(GetFrom(move)))));
		ClearSquare(GetPosition(FILE_H, GetRank(GetFrom(move))));
	}

	if (GetFlag(move) == QUEEN_CASTLE)
	{
		SetSquare(GetPosition(FILE_D, GetRank(GetFrom(move))), GetSquare(GetPosition(FILE_A, GetRank(GetFrom(move)))));
		ClearSquare(GetPosition(FILE_A, GetRank(GetFrom(move))));
	}

	//for some special moves we need to do other things
	switch (GetFlag(move))
	{
	case PAWN_DOUBLE_MOVE:
		SetEnPassant(static_cast<Square>((GetTo(move) + GetFrom(move)) / 2));			//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white
		break;
	case EN_PASSANT:
		ClearSquare(GetPosition(GetFile(GetTo(move)), GetRank(GetFrom(move))));
		break;
	case KNIGHT_PROMOTION:
	case KNIGHT_PROMOTION_CAPTURE:
		SetSquare(GetTo(move), Piece(KNIGHT, GetTurn()));
		break;
	case BISHOP_PROMOTION:
	case BISHOP_PROMOTION_CAPTURE:
		SetSquare(GetTo(move), Piece(BISHOP, GetTurn()));
		break;
	case ROOK_PROMOTION:
	case ROOK_PROMOTION_CAPTURE:
		SetSquare(GetTo(move), Piece(ROOK, GetTurn()));
		break;
	case QUEEN_PROMOTION:
	case QUEEN_PROMOTION_CAPTURE:
		SetSquare(GetTo(move), Piece(QUEEN, GetTurn()));
		break;
	default:
		break;
	}

	if (IsCapture(move) || GetSquare(GetTo(move)) == Piece(PAWN, GetTurn()) || isPromotion(move))
		Reset50Move();

	ClearSquare(GetFrom(move));
	NextTurn();
	UpdateCastleRights(move);
	IncrementZobristKey(move);
	net.ApplyDelta(CalculateMoveDelta(move));

	/*if (GenerateZobristKey() != key)
	{
		std::cout << "error";
		NextTurn();	//just adding something here to really mess things up
	}*/
	//In order to see if the key was corrupted at some point

	/*std::vector<float> delta = CalculateMoveDelta(move);
	std::vector<float> after = GetInputLayer();

	for (int i = 0; i < before.size(); i++)
	{
		if (abs(before[i] + delta[i] - after[i]) > 0.001)
			std::cout << "diff!";
	}*/
	//In order to see if CalculateMoveDelta() works
}

void Position::ApplyMove(const std::string& strmove)
{
	Square prev = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
	Square next = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
	MoveFlag flag = QUIET;

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

		if (IsOccupied(next))				//if it's a capture we need to shift the flag up 4 to turn it from eg: KNIGHT_PROMOTION to KNIGHT_PROMOTION_CAPTURE. EDIT: flag == capture wont work because we just changed the flag!! This was a bug back from 2018 found in 2020
			flag = static_cast<MoveFlag>(flag + CAPTURE);	//might be slow, but don't care.
	}

	ApplyMove(CreateMove(prev, next, flag));
	net.RecalculateIncremental(GetInputLayer());
}

void Position::RevertMove()
{
	assert(PreviousKeys.size() > 0);

	RestorePreviousBoard();
	RestorePreviousParameters();
	key = PreviousKeys.back();
	PreviousKeys.pop_back();
	net.ApplyInverseDelta();
}

void Position::ApplyNullMove()
{
	PreviousKeys.push_back(key);
	SaveParameters();
	SetEnPassant(N_SQUARES);
	SetCaptureSquare(N_SQUARES);
	SetCapturePiece(N_PIECES);
	Increment50Move();

	NextTurn();
	IncrementZobristKey(Move::invalid);

	/*if (GenerateZobristKey() != key)
	{
		std::cout << "error";
		NextTurn();
	}*/
}

void Position::RevertNullMove()
{
	assert(PreviousKeys.size() > 0);

	RestorePreviousParameters();
	key = PreviousKeys.back();
	PreviousKeys.pop_back();
}

void Position::Print() const
{
	std::cout << "\n  A B C D E F G H";

	char Letter[N_SQUARES];

	for (int i = 0; i < N_SQUARES; i++)
	{
		Letter[i] = PieceToChar(GetSquare(static_cast<Square>(i)));
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		unsigned int square = GetPosition(GetFile(i), 7 - GetRank(i));		//7- to flip on the y axis and do rank 8, 7 ...

		if (GetFile(square) == FILE_A)
		{
			std::cout << std::endl;									//Go to a new line
			std::cout << 8 - GetRank(i);							//Count down from 8
		}

		std::cout << " ";
		std::cout << Letter[square];
	}

	std::cout << std::endl;
}

void Position::StartingPosition()
{
	InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
	net.RecalculateIncremental(GetInputLayer());
}

bool Position::InitialiseFromFen(std::vector<std::string> fen)
{
	if (fen.size() == 4)
	{
		fen.push_back("0");
		fen.push_back("1");
	}

	if (fen.size() < 6)
		return false;							//bad fen

	if (!InitialiseBoardFromFen(fen))
		return false;

	if (!InitialiseParametersFromFen(fen))
		return false;

	key = GenerateZobristKey();
	net.RecalculateIncremental(GetInputLayer());

	return true;
}

bool Position::InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle, const std::string& ep, const std::string& fiftyMove, const std::string& turnCount)
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
		if (!stub.empty())
			splitFen[i] = (stub);
	}

	return InitialiseFromFen(splitFen[0], splitFen[1], splitFen[2], splitFen[3], splitFen[4], splitFen[5]);
}

uint64_t Position::GetZobristKey() const
{
	return key;
}

void Position::Reset()
{
	PreviousKeys.clear();
	key = EMPTY;

	ResetBoard();
	InitParameters();
}

uint64_t Position::GenerateZobristKey() const
{
	uint64_t Key = EMPTY;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bitboard = GetPieceBB(static_cast<Pieces>(i));
		while (bitboard != 0)
		{
			Key ^= ZobristTable.at(i * 64 + LSBpop(bitboard));
		}
	}

	if (GetTurn() == WHITE)
		Key ^= ZobristTable.at(12 * 64);

	if (GetCanCastleWhiteKingside())
		Key ^= ZobristTable.at(12 * 64 + 1);
	if (GetCanCastleWhiteQueenside())
		Key ^= ZobristTable.at(12 * 64 + 2);
	if (GetCanCastleBlackKingside())
		Key ^= ZobristTable.at(12 * 64 + 3);
	if (GetCanCastleBlackQueenside())
		Key ^= ZobristTable.at(12 * 64 + 4);

	if (GetEnPassant() <= SQ_H8)
	{
		Key ^= ZobristTable.at(12 * 64 + 5 + GetFile(GetEnPassant()));
	}

	return Key;
}

uint64_t Position::IncrementZobristKey(Move move)
{
	//Change of turn
	key ^= ZobristTable[12 * 64];	//because who's turn it is changed

	if (PrevGetEnPassant() <= SQ_H8)
		key ^= ZobristTable[(12 * 64 + 5 + GetFile(PrevGetEnPassant()))];		//undo the previous ep square

	if (!move) return key;	//null move

	if (!isPromotion(move))
	{
		key ^= ZobristTable[GetSquare(GetTo(move)) * 64 + GetFrom(move)];	//toggle the square we left
		key ^= ZobristTable[GetSquare(GetTo(move)) * 64 + GetTo(move)];	//toggle the arriving square
	}

	//En Passant
	if (GetFlag(move) == EN_PASSANT)
		key ^= ZobristTable[Piece(PAWN, GetTurn()) * 64 + GetPosition(GetFile(GetTo(move)), GetRank(GetFrom(move)))];	//remove the captured piece

	if (GetEnPassant() <= SQ_H8)
		key ^= ZobristTable[12 * 64 + 5 + GetFile(GetEnPassant())];									//apply new EP

	//Captures
	if ((IsCapture(move)) && (GetFlag(move) != EN_PASSANT))
		key ^= ZobristTable[GetCapturePiece() * 64 + GetTo(move)];

	//Castling
	if (GetCanCastleWhiteKingside() != PrevGetCanCastleWhiteKingside())					//if casteling rights changed, flip that one
		key ^= ZobristTable[12 * 64 + 1];
	if (GetCanCastleWhiteQueenside() != PrevGetCanCastleWhiteQueenside())
		key ^= ZobristTable[12 * 64 + 2];
	if (GetCanCastleBlackKingside() != PrevGetCanCastleBlackKingside())
		key ^= ZobristTable[12 * 64 + 3];
	if (GetCanCastleBlackQueenside() != PrevGetCanCastleBlackQueenside())
		key ^= ZobristTable[12 * 64 + 4];

	if (GetFlag(move) == KING_CASTLE)
	{
		key ^= ZobristTable[GetSquare(GetPosition(FILE_F, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_H, GetRank(GetFrom(move)))];
		key ^= ZobristTable[GetSquare(GetPosition(FILE_F, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_F, GetRank(GetFrom(move)))];
	}

	if (GetFlag(move) == QUEEN_CASTLE)
	{
		key ^= ZobristTable[GetSquare(GetPosition(FILE_D, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_A, GetRank(GetFrom(move)))];
		key ^= ZobristTable[GetSquare(GetPosition(FILE_D, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_D, GetRank(GetFrom(move)))];
	}

	//Promotions
	if (isPromotion(move))
	{
		key ^= ZobristTable[Piece(PAWN, !GetTurn()) * 64 + GetFrom(move)];

		if (GetFlag(move) == KNIGHT_PROMOTION || GetFlag(move) == KNIGHT_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(KNIGHT, !GetTurn()) * 64 + GetTo(move)];
		if (GetFlag(move) == BISHOP_PROMOTION || GetFlag(move) == BISHOP_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(BISHOP, !GetTurn()) * 64 + GetTo(move)];
		if (GetFlag(move) == ROOK_PROMOTION || GetFlag(move) == ROOK_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(ROOK, !GetTurn()) * 64 + GetTo(move)];
		if (GetFlag(move) == QUEEN_PROMOTION || GetFlag(move) == QUEEN_PROMOTION_CAPTURE)
			key ^= ZobristTable[Piece(QUEEN, !GetTurn()) * 64 + GetTo(move)];
	}

	return key;
}

std::array<int16_t, INPUT_NEURONS> Position::GetInputLayer() const
{
	std::array<int16_t, INPUT_NEURONS> ret;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bb = GetPieceBB(static_cast<Pieces>(i));

		for (int sq = 0; sq < N_SQUARES; sq++)
		{
			ret[i * 64 + sq] = ((bb & SquareBB[sq]) != 0);
		}
	}

	return ret;
}

deltaArray& Position::CalculateMoveDelta(Move move)
{
	delta.size = 0;
	if (!move) return delta;		//null move

	if (!isPromotion(move))
	{
		delta.deltas[delta.size].index = GetSquare(GetTo(move)) * 64 + GetFrom(move);			//toggle the square we left
		delta.deltas[delta.size++].delta = -1;
		delta.deltas[delta.size].index = GetSquare(GetTo(move)) * 64 + GetTo(move);			//toggle the arriving square
		delta.deltas[delta.size++].delta = 1;
	}

	//En Passant
	if (GetFlag(move) == EN_PASSANT)
	{
		delta.deltas[delta.size].index = Piece(PAWN, GetTurn()) * 64 + GetPosition(GetFile(GetTo(move)), GetRank(GetFrom(move)));	//remove the captured piece
		delta.deltas[delta.size++].delta = -1;
	}

	//Captures
	if ((IsCapture(move)) && (GetFlag(move) != EN_PASSANT))
	{
		delta.deltas[delta.size].index = GetCapturePiece() * 64 + GetTo(move);
		delta.deltas[delta.size++].delta = -1;
	}

	if (GetFlag(move) == KING_CASTLE)
	{
		delta.deltas[delta.size].index = GetSquare(GetPosition(FILE_F, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_H, GetRank(GetFrom(move)));
		delta.deltas[delta.size++].delta = -1;

		delta.deltas[delta.size].index = GetSquare(GetPosition(FILE_F, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_F, GetRank(GetFrom(move)));
		delta.deltas[delta.size++].delta = 1;
	}

	if (GetFlag(move) == QUEEN_CASTLE)
	{
		delta.deltas[delta.size].index = GetSquare(GetPosition(FILE_D, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_A, GetRank(GetFrom(move)));
		delta.deltas[delta.size++].delta = -1;

		delta.deltas[delta.size].index = GetSquare(GetPosition(FILE_D, GetRank(GetFrom(move)))) * 64 + GetPosition(FILE_D, GetRank(GetFrom(move)));
		delta.deltas[delta.size++].delta = 1;
	}

	//Promotions
	if (isPromotion(move))
	{
		delta.deltas[delta.size].index = Piece(PAWN, !GetTurn()) * 64 + GetFrom(move);
		delta.deltas[delta.size++].delta = -1;

		if (GetFlag(move) == KNIGHT_PROMOTION || GetFlag(move) == KNIGHT_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = Piece(KNIGHT, !GetTurn()) * 64 + GetTo(move);
		if (GetFlag(move) == BISHOP_PROMOTION || GetFlag(move) == BISHOP_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = Piece(BISHOP, !GetTurn()) * 64 + GetTo(move);
		if (GetFlag(move) == ROOK_PROMOTION || GetFlag(move) == ROOK_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = Piece(ROOK, !GetTurn()) * 64 + GetTo(move);
		if (GetFlag(move) == QUEEN_PROMOTION || GetFlag(move) == QUEEN_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = Piece(QUEEN, !GetTurn()) * 64 + GetTo(move);

		delta.deltas[delta.size++].delta = 1;
	}

	return delta;
}

void Position::ApplyMoveQuick(Move move)
{
	SaveBoard();

	SetSquare(GetTo(move), GetSquare(GetFrom(move)));
	ClearSquare(GetFrom(move));

	if (GetFlag(move) == EN_PASSANT)
		ClearSquare(GetPosition(GetFile(GetTo(move)), GetRank(GetFrom(move))));
}

void Position::RevertMoveQuick()
{
	RestorePreviousBoard();
}

int16_t Position::GetEvaluation() const
{
	return net.QuickEval();
}

bool Position::CheckForRep(int distanceFromRoot, int maxReps) const
{
	int totalRep = 1;
	uint64_t current = GetZobristKey();

	//note Previous keys will not contain the current key, hence rep starts at one
	for (int i = static_cast<int>(PreviousKeys.size()) - 2; i >= 0; i -= 2)
	{
		if (PreviousKeys[i] == current)
			totalRep++;

		if (totalRep == maxReps) return true;	//maxReps (usually 3) reps is always a draw
		if (totalRep == 2 && static_cast<int>(PreviousKeys.size() - i) < distanceFromRoot - 1)
			return true;						//Don't allow 2 reps if its in the local search history (not part of the actual played game)

		if (GetPreviousFiftyMove(i) == 0) break;
	}

	return false;
}
