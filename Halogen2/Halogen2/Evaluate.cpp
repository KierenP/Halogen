#include "Position.h"

unsigned int PieceNum(uint64_t bb);

const int PawnValue = 100 * 10;
const int KnightValue = 320 * 10;
const int BishopValue = 330 * 10;
const int RookValue = 500 * 10;
const int QueenValue = 900 * 10;
const int KingValue = 10000 * 10;

const int RookSemiOpenFileBonus = 15;
const int ConnectedRookBonus = 30;
const int BishopPairBonus = 20;

const int KnightPawnValue[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 12 }; 		//adjust value based on number of own pawns
const int RookPawnValue	 [9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };		//adjust value based on number of own pawns

const int QueenDevelopPenalty = 70;
const int MiddlePawnPenalty = 35;
const int EdgePawnBonus = 10;
const int KingShieldPawnBonus = 20;
const int CenterPawnBonus = 25;
const int PassedPawnBonus = 25;
const int WeakPawnPenalty = 10;
const int WeakOpenPawnPenalty = 20;
const int DoubledPawnPenalty = 10;

const int UndevelopedPenalty = 30;
const int KnightControlCenterBonus = 20;
const int CenterDiagonalBishopBonus = 20;

int BlackKnightSquareValues[64];
int WhiteKnightSquareValues[64];
int BlackKingSquareOpening[64];
int WhiteKingSquareOpening[64];
int BlackKingSquareEndGame[64];
int WhiteKingSquareEndGame[64];
int BlackBishopSquareValues[64];
int WhiteBishopSquareValues[64];
int BlackRookSquareValues[64];
int WhiteRookSquareValues[64];
int BlackQueenSquareValues[64];
int WhiteQueenSquareValues[64];

int Position::KingSaftey()
{
	int SafetyTable[100] = {
		0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
		18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
		68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
		140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
		260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
		377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
		494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500
	};

	uint64_t AttackTable[N_PIECES];
	GenerateAttackTables(AttackTable);

	unsigned int Threat[N_PIECES] = { 1, 2, 2, 3, 5, 0, 1, 2, 2, 3, 5, 0 };

	uint64_t AttackTableWhite = EMPTY;
	uint64_t AttackTableBlack = EMPTY;

	unsigned int WhiteAttacks = 0;											//white attacks AGAINST the black king. Higher number good for white
	unsigned int BlackAttacks = 0;

	for (int i = 0; i < N_PIECES; i++)
	{
		if (i <= BLACK_KING)
			AttackTableBlack |= AttackTable[i];
		else
			AttackTableWhite |= AttackTable[i];
	}
	
	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t ThreatBB = EMPTY;

		if (i <= BLACK_KING)
			ThreatBB = (AttackTable[i] & ~(AttackTableWhite)) & WhiteKingAttackSquares[GetKing(WHITE)];					//Get the threats (where the black piece can move) that are not defented by a white piece
		else
			ThreatBB = (AttackTable[i] & ~(AttackTableBlack)) & BlackKingAttackSquares[GetKing(BLACK)];

		while (ThreatBB != 0)
		{
			bitScanFowardErase(ThreatBB);
			if (i <= BLACK_KING)
				BlackAttacks += Threat[i];										//Add the threat of that piece to the number of attacks
			else
				WhiteAttacks += Threat[i];
		}
	}

	unsigned int mobility = 0;
	AttackTable[WHITE_QUEEN] = EMPTY;
	AttackTable[BLACK_QUEEN] = EMPTY;

	for (int i = WHITE_PAWN; i < WHITE_KING; i++)
	{
		for (; AttackTable[i] != 0; bitScanFowardErase(AttackTable[i]))
			mobility++;
	}

	for (int i = BLACK_PAWN; i < BLACK_KING; i++)
	{
		for (; AttackTable[i] != 0; bitScanFowardErase(AttackTable[i]))
			mobility--;
	}

	return (SafetyTable[WhiteAttacks] - SafetyTable[BlackAttacks]) + (mobility * 5) + ((BoardParamiter.CurrentTurn * 2 - 1) * 10);
}

int Position::Evaluate()
{
	int Score = 0;

	if (BoardParamiter.GameStage != ENDGAME)
	{
		uint64_t White = GetPieces(WHITE_KNIGHT) | GetPieces(WHITE_BISHOP);
		while (White != 0)
		{
			unsigned int pos = bitScanFowardErase(White);
			if (GetRank(pos) == RANK_1)
				Score -= UndevelopedPenalty;
		}

		uint64_t Black = GetPieces(BLACK_KNIGHT) | GetPieces(BLACK_BISHOP);
		while (Black != 0)
		{
			unsigned int pos = bitScanFowardErase(Black);
			if (GetRank(pos) == RANK_8)
				Score += UndevelopedPenalty;
		}
	}

	uint64_t whiterooks = GetPieces(WHITE_ROOK);
	while (whiterooks != 0)
	{
		unsigned int pos = bitScanFowardErase(whiterooks);
		Score += EvalRook(pos, WHITE);
	}

	uint64_t blackrooks = GetPieces(BLACK_ROOK);
	while (blackrooks != 0)
	{
		unsigned int pos = bitScanFowardErase(blackrooks);
		Score += EvalRook(pos, BLACK);
	}

	uint64_t whitebishops = GetPieces(WHITE_BISHOP);
	while (whitebishops != 0)
	{
		unsigned int pos = bitScanFowardErase(whitebishops);
		Score += EvalBishop(pos, WHITE);
	}

	uint64_t blackbishops = GetPieces(BLACK_BISHOP);
	while (blackbishops != 0)
	{
		unsigned int pos = bitScanFowardErase(blackbishops);
		Score += EvalBishop(pos, BLACK);
	}

	uint64_t whitequeens = GetPieces(WHITE_QUEEN);
	while (whitequeens != 0)
	{
		unsigned int pos = bitScanFowardErase(whitequeens);
		Score += EvalQueen(pos, WHITE);
	}

	uint64_t blackqueens = GetPieces(BLACK_QUEEN);
	while (blackqueens != 0)
	{
		unsigned int pos = bitScanFowardErase(blackqueens);
		Score += EvalQueen(pos, BLACK);
	}

	uint64_t whiteknights = GetPieces(WHITE_KNIGHT);
	while (whiteknights != 0)
	{
		unsigned int pos = bitScanFowardErase(whiteknights);
		Score += EvalKnight(pos, WHITE);
	}

	uint64_t blackknights = GetPieces(BLACK_KNIGHT);
	while (blackknights != 0)
	{
		unsigned int pos = bitScanFowardErase(blackknights);
		Score += EvalKnight(pos, BLACK);
	}

	Score += EvalKing(GetKing(WHITE), WHITE);
	Score += EvalKing(GetKing(BLACK), BLACK);
	Score += PawnStrucureEval();

	if (PieceNum(WHITE_BISHOP) > 1)		//bishop pair bonus
		Score += BishopPairBonus;

	if (PieceNum(BLACK_BISHOP) > 1)
		Score -= BishopPairBonus;

	if (BoardParamiter.GameStage != ENDGAME)
		Score += KingSaftey();

	return Score / 10;
}

int Position::EvalKnight(unsigned int sq, unsigned int colour)
{
	int centerAttackBonus = 0;

	if ((KnightAttacks[sq] & (SquareBB[SQ_E4] | SquareBB[SQ_E5] | SquareBB[SQ_D4] | SquareBB[SQ_D5])) != 0)
		centerAttackBonus = KnightControlCenterBonus * 2;

	if (colour == WHITE)
		return +(WhiteKnightSquareValues[sq] + KnightPawnValue[PieceNum(GetPieces(Piece(PAWN, colour)))] + KnightValue + centerAttackBonus);
	if (colour == BLACK)
		return -(BlackKnightSquareValues[sq] + KnightPawnValue[PieceNum(GetPieces(Piece(PAWN, colour)))] + KnightValue + centerAttackBonus);

	return 0;
}

int Position::EvalBishop(unsigned int sq, unsigned int colour)
{
	int diagonalbishop = 0;
	if (GetDiagonal(sq) == DIAG_A8H1 || GetDiagonal(sq) == DIAG_A7G1 || GetDiagonal(sq) == DIAG_B8H2 || GetAntiDiagonal(sq) == DIAG_A2G8 || GetAntiDiagonal(sq) == DIAG_A1H8 || GetAntiDiagonal(sq) == DIAG_B1H7)
		diagonalbishop = CenterDiagonalBishopBonus * 2;

	if (colour == WHITE)
		return +(WhiteBishopSquareValues[sq] + BishopValue + diagonalbishop);
	if (colour == BLACK)
		return -(BlackBishopSquareValues[sq] + BishopValue + diagonalbishop);

	return 0;
}

int Position::EvalRook(unsigned int sq, unsigned int colour)
{
	int OpenFileBonus = 0;
	int ConnectionBonus = 0;

	if (GetGameState() != OPENING)
	{
		if ((PieceNum(WHITE_PAWN) & FileBB[GetFile(sq)]) == 0)			//semi (or full) open file
			OpenFileBonus += OpenFileBonus;
		if ((PieceNum(BLACK_PAWN) & FileBB[GetFile(sq)]) == 0)			//semi (or full) open file
			OpenFileBonus += OpenFileBonus;
	}

	if ((GetGameState() != ENDGAME) && (PieceNum(GetPieces(Piece(ROOK, colour))) == 2))
	{
		uint64_t rooks = GetPieces(Piece(ROOK, colour)) ^ SquareBB[sq];
		unsigned int other = bitScanForward(rooks);

		if (colour == WHITE && ((GetPieces(WHITE_ROOK) & RankBB[RANK_1]) == GetPieces(WHITE_ROOK)) && ((inBetweenCache(sq, other) & AllPieces()) == 0))
			ConnectionBonus += ConnectedRookBonus;
		if (colour == BLACK && ((GetPieces(BLACK_ROOK) & RankBB[RANK_8]) == GetPieces(BLACK_ROOK)) && ((inBetweenCache(sq, other) & AllPieces()) == 0))
			ConnectionBonus += ConnectedRookBonus;
	}

	if (colour == WHITE)
		return +(WhiteRookSquareValues[sq] + RookPawnValue[PieceNum(GetPieces(Piece(PAWN, colour)))] + OpenFileBonus + RookValue + ConnectionBonus);
	if (colour == BLACK)
		return -(BlackRookSquareValues[sq] + RookPawnValue[PieceNum(GetPieces(Piece(PAWN, colour)))] + OpenFileBonus + RookValue + ConnectionBonus);

	return 0;
}

int Position::EvalQueen(unsigned int sq, unsigned int colour)
{
	int DevelopPenalty = 0;

	if (colour == WHITE && GetRank(sq) >= RANK_2 && GetGameState() == OPENING)		
		DevelopPenalty -= QueenDevelopPenalty;
	if (colour == BLACK && GetRank(sq) <= RANK_7 && GetGameState() == OPENING)
		DevelopPenalty -= QueenDevelopPenalty;

	if (colour == WHITE)
		return +(WhiteQueenSquareValues[sq] + DevelopPenalty + QueenValue);
	if (colour == BLACK)
		return -(BlackQueenSquareValues[sq] + DevelopPenalty + QueenValue);

	return 0;
}

int Position::EvalKing(unsigned int sq, unsigned int colour)
{
	int castlebonus = 0;

	if (colour == WHITE)
	{
		if (BoardParamiter.HasCastledWhite)
			castlebonus += 40;
	}

	if (colour == BLACK)
	{
		if (BoardParamiter.HasCastledBlack)
			castlebonus += 40;
	}


	if (colour == WHITE)
		return +(WhiteKingSquareOpening[sq] + KingValue + castlebonus);
	if (colour == BLACK)
		return -(BlackKingSquareOpening[sq] + KingValue + castlebonus);

	return 0;
}

int Position::PawnStrucureEval()
{
	int result = 0;
	uint64_t WhitePawns = GetPieces(WHITE_PAWN);
	uint64_t BlackPawns = GetPieces(BLACK_PAWN);

	while (WhitePawns != 0)
	{
		unsigned int start = bitScanFowardErase(WhitePawns);
		result += EvalPawn(start, WHITE);
	}

	while (BlackPawns != 0)
	{
		unsigned int start = bitScanFowardErase(BlackPawns);
		result -= EvalPawn(start, BLACK);
	}

	if (GetSquare(SQ_E2) == WHITE_PAWN && GetSquare(SQ_D2) == WHITE_PAWN)
		result -= MiddlePawnPenalty;

	if (GetSquare(SQ_E7) == BLACK_PAWN && GetSquare(SQ_D7) == BLACK_PAWN)
		result += MiddlePawnPenalty;

	if (GetGameState() != ENDGAME)
	{
		if (GetFile(GetKing(WHITE)) < FILE_C)			//castled queen side
		{
			unsigned int ShieldPawnScore = 0;
			if (GetSquare(SQ_A2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else 
			{
				if (GetSquare(SQ_A3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_B2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_B3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_C2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_C3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			result += ((ShieldPawnScore + 1) / 2) * KingShieldPawnBonus;						//5 or 6 produces the maximum 3x, 3-4 2x and 1-2 1x. This allows one pawn to be pushed with no penalty.
		}

		if (GetFile(GetKing(WHITE)) > FILE_F)			//castled king side
		{
			unsigned int ShieldPawnScore = 0;
			if (GetSquare(SQ_F2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_F3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_G2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_G3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_H2) == WHITE_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_H3) == WHITE_PAWN)
					ShieldPawnScore += 1;
			}
			result += ((ShieldPawnScore + 1) / 2) * KingShieldPawnBonus;						//5 or 6 produces the maximum 3x, 3-4 2x and 1-2 1x. This allows one pawn to be pushed with no penalty.
		}

		if (GetFile(GetKing(BLACK)) < FILE_C)			//castled queen side
		{
			unsigned int ShieldPawnScore = 0;
			if (GetSquare(SQ_A7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_A6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_B7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_B6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_C7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_C6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			result -= ((ShieldPawnScore + 1) / 2) * KingShieldPawnBonus;						//5 or 6 produces the maximum 3x, 3-4 2x and 1-2 1x. This allows one pawn to be pushed with no penalty.
		}

		if (GetFile(GetKing(BLACK)) > FILE_F)			//castled king side
		{
			unsigned int ShieldPawnScore = 0;
			if (GetSquare(SQ_F7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_F6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_G7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_G6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			if (GetSquare(SQ_H7) == BLACK_PAWN)
				ShieldPawnScore += 2;
			else
			{
				if (GetSquare(SQ_H6) == BLACK_PAWN)
					ShieldPawnScore += 1;
			}
			result -= ((ShieldPawnScore + 1) / 2) * KingShieldPawnBonus;						//5 or 6 produces the maximum 3x, 3-4 2x and 1-2 1x. This allows one pawn to be pushed with no penalty.
		}

		if (GetSquare(SQ_E4) == WHITE_PAWN)
			result += CenterPawnBonus;

		if (GetSquare(SQ_E4) == BLACK_PAWN)
			result -= CenterPawnBonus;

		if (GetSquare(SQ_D4) == WHITE_PAWN)
			result += CenterPawnBonus;

		if (GetSquare(SQ_D4) == BLACK_PAWN)
			result -= CenterPawnBonus;

		if (GetSquare(SQ_E5) == WHITE_PAWN)
			result += CenterPawnBonus;

		if (GetSquare(SQ_E5) == BLACK_PAWN)
			result -= CenterPawnBonus;

		if (GetSquare(SQ_D5) == WHITE_PAWN)
			result += CenterPawnBonus;

		if (GetSquare(SQ_D5) == BLACK_PAWN)
			result -= CenterPawnBonus;
	}

	return result;
}

int Position::EvalPawn(unsigned int position, unsigned int colour)
{
	int result = 0;
	bool IsPassed = false;
	bool IsWeak = true;
	bool IsOpposed = true;
	bool IsDoubled = false;

	int foward, back;

	if (colour == WHITE) { foward = 8; back = -8; }
	if (colour == BLACK) { foward = -8; back = 8; }

	if ((colour == WHITE) && ((GetPieces(Piece(PAWN, BLACK)) & PassedPawnMaskWhite[position]) == 0))
		IsPassed = true;
	if ((colour == BLACK) && ((GetPieces(Piece(PAWN, WHITE)) & PassedPawnMaskBlack[position]) == 0))
		IsPassed = true;

	if (((GetPieces(Piece(PAWN, colour)) && FileBB[GetFile(position)]) ^ SquareBB[position]) != 0)
		IsDoubled = true;

	IsOpposed = !IsPassed;

	uint64_t friendlypawns = GetPieces(Piece(PAWN, colour));
	uint64_t mask = 0;

	if (GetFile(position) != FILE_A)
		mask |= FileBB[GetFile(position - 1)];
	if (GetFile(position) != FILE_H)
		mask |= FileBB[GetFile(position + 1)];
	
	mask &= (RankBB[GetRank(position)] | RankBB[GetRank(position + back)]);	//mask is squares in files either side, and ranks equal or behind 1

	if ((friendlypawns & mask) != 0)
		IsWeak = false;

	if (IsPassed)
		result += PassedPawnBonus;

	if (IsWeak)
	{
		if (IsOpposed)
			result -= WeakPawnPenalty;
		else
			result -= WeakOpenPawnPenalty;
	}

	if (IsDoubled)
		result -= DoubledPawnPenalty;

	result += PawnValue;

	return result;
}

int Position::CheckMateScore()
{
	return -((BoardParamiter.CurrentTurn * 2 - 1) * (9500));			//-9500 for white, 9500 for black
}

unsigned int PieceNum(uint64_t bb)
{
	unsigned int num = 0;

	while (bb != 0)
	{
		bitScanFowardErase(bb);
		num++;
	}

	return num;
}

void Position::EvalInit()
{
	int BlackKnightSquareValues[64] = {
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0, 10, 15, 15, 10,  0,-30,
		-30,  5, 15, 20, 20, 15,  5,-30,
		-30,  0, 15, 20, 20, 15,  0,-30,
		-30,  5, 10, 15, 15, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-40,-30,-30,-30,-30,-40,-50,
	};

	int WhiteKnightSquareValues[64] = {
		-50,-40,-30,-30,-30,-30,-40,-50,	//1
		-40,-20,  0,  5,  5,  0,-20,-40,	//2
		-30,  5, 10, 15, 15, 10,  5,-30,	//3
		-30,  0, 15, 20, 20, 15,  0,-30,	//4
		-30,  5, 15, 20, 20, 15,  5,-30,	//5
		-30,  0, 10, 15, 15, 10,  0,-30,	//6
		-40,-20,  0,  0,  0,  0,-20,-40,	//7
		-50,-40,-30,-30,-30,-30,-40,-50	    //8
	};

	int BlackKingSquareOpening[64] = {
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		20, 20,  0,  0,  0,  0, 20, 20,
		20, 30, 10,  0,  0, 10, 30, 20
	};

	int WhiteKingSquareOpening[64] = {
		20, 30, 10,  0,  0, 10, 30, 20,
		20, 20,  0,  0,  0,  0, 20, 20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
	};

	int BlackKingSquareEndGame[64] = {
		-50,-30,-30,-30,-30,-30,-30,-50
		- 30,-30,  0,  0,  0,  0,-30,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-50,-40,-30,-20,-20,-30,-40,-50,
	};

	int WhiteKingSquareEndGame[64] = {
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  0,  0,  0,  0,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50
	};

	int BlackBishopSquareValues[64] = {
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10, 20,  0,  0,  0,  0,  20,-10,
		-20,-10,-10,-10,-10,-10,-10,-20
	};

	int WhiteBishopSquareValues[64] = {
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  20,  0,  0,  0,  0,  20,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10,  5,  5, 10, 10,  5,  5,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10,-10,-10,-10,-10,-20
	};

	int BlackRookSquareValues[64] = {
		0,  0,  0,  0,  0,  0,  0,  0,
		5, 20, 20, 20, 20, 20, 20,  5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		0,  0,  0,  5,  5,  0,  0,  0
	};

	int WhiteRookSquareValues[64] = {
		0,  0,  0,  0,  0,  0,  0,  0,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		5, 20, 20, 20, 20, 20, 20,  5,
		0,  0,  0,  0,  0,  0,  0,  0,
	};

	int BlackQueenSquareValues[64] = {
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		-5,  0,  5,  5,  5,  5,  0, -5,
		0,  0,  5,  5,  5,  5,  0, -5,
		-10,  5,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10, 0, -5,-10,-10,-20
	};

	int WhiteQueenSquareValues[64] = {
		-20,-10,-10, 0, -5,-10,-10,-20
		- 10,  0,  5,  0,  0,  0,  0,-10,
		-10,  5,  5,  5,  5,  5,  0,-10,
		0,  0,  5,  5,  5,  5,  0, -5,
		-5,  0,  5,  5,  5,  5,  0, -5,
		-10,  0,  5,  5,  5,  5,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10, -5, -5,-10,-10,-20
	};
}

bool Position::IsCheckmate()
{
	GenerateLegalMoves();

	if (LegalMoves.size() == 0 && IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn))
		return true;
	return false;
}
