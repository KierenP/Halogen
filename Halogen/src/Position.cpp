#include "Position.h"

Position::Position()
{
	key = EMPTY;
	StartingPosition();
}

Position::~Position()
{

}

void Position::ApplyMove(Move move)
{
	PreviousKeys.push_back(key);
	SaveParameters();
	SaveBoard();
	SetEnPassant(N_SQUARES);
	Increment50Move();

	if (move.IsCapture())
	{
		SetCaptureSquare(move.GetTo());
		SetCapturePiece(GetSquare(move.GetTo()));	//will fail with en passant, but that currently doesn't matter
	}
	else
	{
		SetCaptureSquare(N_SQUARES);
		SetCapturePiece(N_PIECES);
	}

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
		SetEnPassant(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2));			//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white
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

	if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, GetTurn()) || move.IsPromotion())
		Reset50Move();

	ClearSquare(move.GetFrom());
	NextTurn();
	UpdateCastleRights(move);
	IncrementZobristKey(move);
	ApplyDelta(CalculateMoveDelta(move), Zeta);

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

void Position::ApplyMove(std::string strmove)
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

	ApplyMove(Move(prev, next, flag));
	RecalculateIncremental(GetInputLayer(), Zeta);
}

void Position::RevertMove()
{
	assert(PreviousKeys.size() > 0);

	RestorePreviousBoard();
	RestorePreviousParameters();
	key = PreviousKeys.back();
	PreviousKeys.pop_back();
	ApplyInverseDelta(Zeta);
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
	RecalculateIncremental(GetInputLayer(), Zeta);
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
	RecalculateIncremental(GetInputLayer(), Zeta);

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

uint64_t Position::GetZobristKey() const
{
	return key;
}

void Position::Reset()
{
	PreviousKeys.clear();
	key = EMPTY;
	EvaluatedPositions = 0;

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
		key ^= ZobristTable[GetCapturePiece() * 64 + move.GetTo()];

	//Castling
	if (GetCanCastleWhiteKingside() != PrevGetCanCastleWhiteKingside())					//if casteling rights changed, flip that one
		key ^= ZobristTable[12 * 64 + 1];
	if (GetCanCastleWhiteQueenside() != PrevGetCanCastleWhiteQueenside())
		key ^= ZobristTable[12 * 64 + 2];
	if (GetCanCastleBlackKingside() != PrevGetCanCastleBlackKingside())
		key ^= ZobristTable[12 * 64 + 3];
	if (GetCanCastleBlackQueenside() != PrevGetCanCastleBlackQueenside())
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

std::array<int16_t, INPUT_NEURONS> Position::GetInputLayer() const
{
	std::array<int16_t, INPUT_NEURONS> ret;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bb = GetPieceBB(static_cast<Pieces>(i));

		for (int sq = 0; sq < N_SQUARES; sq++)
		{
			ret[modifier(i * 64 + sq)] = ((bb & SquareBB[sq]) != 0);
		}
	}

	return ret;
}

deltaArray& Position::CalculateMoveDelta(Move move)
{
	delta.size = 0;
	if (move.IsUninitialized()) return delta;		//null move

	if (!move.IsPromotion())
	{
		delta.deltas[delta.size].index = modifier(GetSquare(move.GetTo()) * 64 + move.GetFrom());			//toggle the square we left
		delta.deltas[delta.size++].delta = -1;
		delta.deltas[delta.size].index = modifier(GetSquare(move.GetTo()) * 64 + move.GetTo());			//toggle the arriving square
		delta.deltas[delta.size++].delta = 1;
	}

	//En Passant
	if (move.GetFlag() == EN_PASSANT)
	{
		delta.deltas[delta.size].index = modifier(Piece(PAWN, GetTurn()) * 64 + GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));	//remove the captured piece
		delta.deltas[delta.size++].delta = -1;
	}

	//Captures
	if ((move.IsCapture()) && (move.GetFlag() != EN_PASSANT))
	{
		delta.deltas[delta.size].index = modifier(GetCapturePiece() * 64 + move.GetTo());
		delta.deltas[delta.size++].delta = -1;
	}

	if (move.GetFlag() == KING_CASTLE)
	{
		delta.deltas[delta.size].index = modifier(GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_H, GetRank(move.GetFrom())));
		delta.deltas[delta.size++].delta = -1;

		delta.deltas[delta.size].index = modifier(GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_F, GetRank(move.GetFrom())));
		delta.deltas[delta.size++].delta = 1;
	}

	if (move.GetFlag() == QUEEN_CASTLE)
	{
		delta.deltas[delta.size].index = modifier(GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_A, GetRank(move.GetFrom())));
		delta.deltas[delta.size++].delta = -1;

		delta.deltas[delta.size].index = modifier(GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))) * 64 + GetPosition(FILE_D, GetRank(move.GetFrom())));
		delta.deltas[delta.size++].delta = 1;
	}

	//Promotions
	if (move.IsPromotion())
	{
		delta.deltas[delta.size].index = modifier(Piece(PAWN, !GetTurn()) * 64 + move.GetFrom());
		delta.deltas[delta.size++].delta = -1;

		if (move.GetFlag() == KNIGHT_PROMOTION || move.GetFlag() == KNIGHT_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = modifier(Piece(KNIGHT, !GetTurn()) * 64 + move.GetTo());
		if (move.GetFlag() == BISHOP_PROMOTION || move.GetFlag() == BISHOP_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = modifier(Piece(BISHOP, !GetTurn()) * 64 + move.GetTo());
		if (move.GetFlag() == ROOK_PROMOTION || move.GetFlag() == ROOK_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = modifier(Piece(ROOK, !GetTurn()) * 64 + move.GetTo());
		if (move.GetFlag() == QUEEN_PROMOTION || move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
			delta.deltas[delta.size].index = modifier(Piece(QUEEN, !GetTurn()) * 64 + move.GetTo());

		delta.deltas[delta.size++].delta = 1;
	}

	return delta;
}

size_t Position::modifier(size_t index)
{
	if (index >= 384)
	{
		return index - 384;
	}
	else
	{
		return index + 384;
	}
}

void Position::ApplyMoveQuick(Move move)
{
	SaveBoard();

	SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
	ClearSquare(move.GetFrom());

	if (move.GetFlag() == EN_PASSANT)
		ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
}

void Position::RevertMoveQuick()
{
	RestorePreviousBoard();
}

int16_t Position::GetEvaluation()
{
	return QuickEval(Zeta);
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
