#include "Evaluate.h"

const int RookSemiOpenFileBonus = 15;
const int ConnectedRookBonus = 30;
const int BishopPairBonus = 20;

const unsigned int PieceValues[N_PIECES] = { 100, 320, 330, 500, 900, 10000, 100, 320, 330, 500, 900, 10000 };
const int KnightPawnValue[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 12 }; 		//adjust value based on number of enemy pawns
const int RookPawnValue[9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };			//adjust value based on number of own pawns

const int QueenDevelopPenalty = 50;
const int PassedPawnBonus = 25;
const int WeakPawnPenalty = 10;
const int WeakOpenPawnPenalty = 20;
const int DoubledPawnPenalty = 10;

const int UndevelopedPenalty = 30;
const int KnightControlCenterBonus = 10;

const int CastledBonus = 40;
const int SafetyTable[100] = {
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

const unsigned int Threat[N_PIECES] = { 1, 2, 2, 3, 5, 0, 1, 2, 2, 3, 5, 0 };

unsigned int CalculateGameStage(const Position& position);
int PieceDevelopment(const Position& position);
int EvaluateCastleBonus(const Position& position);
int EvaluatePawn(const Position& position, unsigned int square, bool colour);
int EvaluatePawnStructure(const Position& position);
int EvaluatePieceSquareTables(const Position& position, unsigned int gameStage);
//int EvaluateKnightCenterControl(const Position& position);
//int EvaluateKnightPawnModifier(const Position& position);
int EvaluateMaterial(const Position& position);

/*int PositionTempName::KingSaftey()
{
	unsigned int WhiteAttacks = 0;											//white attacks AGAINST the black king. Higher number good for white
	unsigned int BlackAttacks = 0;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t ThreatBB = EMPTY;

		if (i <= BLACK_KING)
			ThreatBB = (AttackTable[i] & ~(WhiteThreats)) & WhiteKingAttackSquares[GetKing(WHITE)];					//Get the threats (where the black piece can move) that are not defented by a white piece
		else
			ThreatBB = (AttackTable[i] & ~(BlackThreats)) & BlackKingAttackSquares[GetKing(BLACK)];

		while (ThreatBB != 0)
		{
			bitScanFowardErase(ThreatBB);
			if (i <= BLACK_KING)
				BlackAttacks += Threat[i];										//Add the threat of that piece to the number of attacks
			else
				WhiteAttacks += Threat[i];
		}
	}

	/*unsigned int mobility = 0;
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

	return (SafetyTable[WhiteAttacks] - SafetyTable[BlackAttacks]);
}*/

int EvaluatePosition(const Position & position)
{
	int Score = 0;
	unsigned int GameStage = CalculateGameStage(position);

	//Score += PieceDevelopment(position);
	//Score += CastleBonus(position);
	//Score += EvaluatePieceSquareTables(position, GameStage);
	//Score += EvaluatePawnStructure(position);
	//Score += EvaluateKnightCenterControl(position, GameStage);

	int Material = EvaluateMaterial(position);
	int PieceSquares = EvaluatePieceSquareTables(position, GameStage);
	int PawnStructure = EvaluatePawnStructure(position);
	int Castle = EvaluateCastleBonus(position);

	/*std::cout << "Material: " << Material << "\n";
	std::cout << "Piece Squares: " << PieceSquares << "\n";
	std::cout << "Pawn Structure: " << PawnStructure << "\n";
	std::cout << "Castle Bonus: " << Castle << "\n";
	
	Score += Material + PieceSquares + PawnStructure + Castle;

	std::cout << "Total: " << Score << "\n";*/

	//int Bishops = Evaluate
	
	//KnightPawnValueMod
	//Rook open file 
	//Connected rooks
	//RookPawnValueMod
	//bishoppairbonus
	//king saftey

	Score += Material + PieceSquares + PawnStructure + Castle;
	return Score;
}

void InitializeEvaluation()
{
	InitializePieceSquareTable();
}

/*int PositionTempName::EvalKnight(unsigned int sq, unsigned int colour)
{
	if (colour == WHITE)
		return +(WhiteKnightSquareValues[sq] + KnightPawnValue[PieceNum(position.GetPieceBB(PAWN, colour))] + KnightValue + centerAttackBonus);
	if (colour == BLACK)
		return -(BlackKnightSquareValues[sq] + KnightPawnValue[PieceNum(position.GetPieceBB(PAWN, colour))] + KnightValue + centerAttackBonus);

	return 0;
}

int PositionTempName::EvalRook(unsigned int sq, unsigned int colour)
{
	int OpenFileBonus = 0;
	int ConnectionBonus = 0;

	if (GetGameState() != OPENING)
	{
		if ((position.GetPieceBB(WHITE_PAWN) & FileBB[GetFile(sq)]) == 0)			//semi (or full) open file
			OpenFileBonus += OpenFileBonus;
		if ((position.GetPieceBB(BLACK_PAWN) & FileBB[GetFile(sq)]) == 0)			//semi (or full) open file
			OpenFileBonus += OpenFileBonus;
	}

	if ((GetGameState() != ENDGAME) && (PieceNum(position.GetPieceBB(ROOK, colour)) == 2))
	{
		uint64_t rooks = position.GetPieceBB(ROOK, colour) ^ SquareBB[sq];
		unsigned int other = bitScanForward(rooks);

		if (colour == WHITE && ((position.GetPieceBB(WHITE_ROOK) & RankBB[RANK_1]) == position.GetPieceBB(WHITE_ROOK)) && ((inBetweenCache(sq, other) & AllPieces()) == 0))
			ConnectionBonus += ConnectedRookBonus;
		if (colour == BLACK && ((position.GetPieceBB(BLACK_ROOK) & RankBB[RANK_8]) == position.GetPieceBB(BLACK_ROOK)) && ((inBetweenCache(sq, other) & AllPieces()) == 0))
			ConnectionBonus += ConnectedRookBonus;
	}

	if (colour == WHITE)
		return +(WhiteRookSquareValues[sq] + RookPawnValue[PieceNum(position.GetPieceBB(PAWN, colour))] + OpenFileBonus + RookValue + ConnectionBonus);
	if (colour == BLACK)
		return -(BlackRookSquareValues[sq] + RookPawnValue[PieceNum(position.GetPieceBB(PAWN, colour))] + OpenFileBonus + RookValue + ConnectionBonus);

	return 0;
}*/

unsigned int CalculateGameStage(const Position & position)
{
	if (position.GetTurnCount() < 10)
		return OPENING;

	if (position.GetTurnCount() > 25)
		return ENDGAME;

	return MIDGAME;
}

int PieceDevelopment(const Position & position)
{
	int score = 0;

	if (CalculateGameStage(position) != ENDGAME)
	{
		score -= GetBitCount((position.GetPieceBB(WHITE_KNIGHT) | position.GetPieceBB(WHITE_BISHOP)) & RankBB[RANK_1]) * UndevelopedPenalty;
		score += GetBitCount((position.GetPieceBB(BLACK_KNIGHT) | position.GetPieceBB(BLACK_BISHOP)) & RankBB[RANK_8]) * UndevelopedPenalty;
	}

	if (CalculateGameStage(position) == OPENING)
	{
		if ((position.GetPieceBB(WHITE_QUEEN) & ~RankBB[RANK_1]) != 0)
			score -= QueenDevelopPenalty;
		if ((position.GetPieceBB(BLACK_QUEEN) & ~RankBB[RANK_8]) != 0)
			score += QueenDevelopPenalty;
	}

	return score;
}

int EvaluateCastleBonus(const Position & position)
{
	int score = 0;

	if (position.HasCastledWhite())
		score += CastledBonus;

	if (position.HasCastledBlack())
		score -= CastledBonus;

	return score;
}

int EvaluatePawn(const Position & position, const unsigned int square, const bool colour)
{
	int result = 0;
	bool IsPassed = false;
	bool IsWeak = true;
	bool IsOpposed = false;
	bool IsDoubled = false;

	int foward, back;

	if (colour == WHITE) { foward = 8; back = -8; }
	if (colour == BLACK) { foward = -8; back = 8; }

	if ((colour == WHITE) && (((position.GetPieceBB(PAWN, BLACK)) & PassedPawnMaskWhite[square]) == 0))
		IsPassed = true;
	if ((colour == BLACK) && (((position.GetPieceBB(PAWN, WHITE)) & PassedPawnMaskBlack[square]) == 0))
		IsPassed = true;

	if ((((position.GetPieceBB(PAWN, colour)) & FileBB[GetFile(square)]) ^ SquareBB[square]) != 0)
		IsDoubled = true;

	if (((position.GetPieceBB(PAWN, !colour)) & FileBB[GetFile(square)]) != 0)
		IsOpposed = true;

	uint64_t friendlypawns = position.GetPieceBB(PAWN, colour);
	uint64_t mask = 0;

	if (GetFile(square) != FILE_A)
		mask |= FileBB[GetFile(square) - 1];
	if (GetFile(square) != FILE_H)
		mask |= FileBB[GetFile(square) + 1];

	mask &= (RankBB[GetRank(square)] | RankBB[GetRank(square + back)]);										//mask is squares in files either side, and ranks equal or behind 1

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

	return result;
}

int EvaluatePawnStructure(const Position & position)
{
	int result = 0;

	for (uint64_t WhitePawns = position.GetPieceBB(WHITE_PAWN); WhitePawns != 0; result += EvaluatePawn(position, bitScanFowardErase(WhitePawns), WHITE));
	for (uint64_t BlackPawns = position.GetPieceBB(BLACK_PAWN); BlackPawns != 0; result -= EvaluatePawn(position, bitScanFowardErase(BlackPawns), BLACK));

	return result;
}

int EvaluatePieceSquareTables(const Position & position, unsigned int gameStage)
{
	int Score = 0;

	for (int i = 0; i < N_PIECE_TYPES; i++)
	{
		for (uint64_t piece = position.GetPieceBB(i); piece != 0; Score -= PieceSquareTables[gameStage][i][bitScanFowardErase(piece)]);														//black piece
		for (uint64_t piece = position.GetPieceBB(i + N_PIECE_TYPES); piece != 0; Score += PieceSquareTables[gameStage][i + N_PIECE_TYPES][bitScanFowardErase(piece)]);						//white piece
	}

	return Score;
}

/*int EvaluateKnightCenterControl(const Position & position)
{
	int Score = 0;

	for (uint64_t piece = position.GetPieceBB(WHITE_KNIGHT); piece != 0; Score += KnightControlCenterBonus * GetBitCount(KnightAttacks[bitScanFowardErase(piece)] & (SquareBB[SQ_E4] | SquareBB[SQ_E5] | SquareBB[SQ_D4] | SquareBB[SQ_D5])));
	for (uint64_t piece = position.GetPieceBB(BLACK_KNIGHT); piece != 0; Score -= KnightControlCenterBonus * GetBitCount(KnightAttacks[bitScanFowardErase(piece)] & (SquareBB[SQ_E4] | SquareBB[SQ_E5] | SquareBB[SQ_D4] | SquareBB[SQ_D5])));

	return Score;
}

int EvaluateKnightPawnModifier(const Position & position)
{
	int Score = 0;

	Score += GetBitCount(position.GetPieceBB(WHITE_KNIGHT)) * KnightPawnValue[GetBitCount(position.GetPieceBB(BLACK_PAWN))];
	Score -= GetBitCount(position.GetPieceBB(BLACK_KNIGHT)) * KnightPawnValue[GetBitCount(position.GetPieceBB(WHITE_PAWN))];

	return Score;
}*/

int EvaluateMaterial(const Position & position)
{
	int Score = 0;

	for (int i = 0; i < N_PIECE_TYPES; i++)
	{
		for (uint64_t piece = position.GetPieceBB(i); piece != 0; Score -= PieceValues[i]) bitScanFowardErase(piece);										//black piece
		for (uint64_t piece = position.GetPieceBB(i + N_PIECE_TYPES); piece != 0; Score += PieceValues[i + N_PIECE_TYPES]) bitScanFowardErase(piece);		//white piece
	}

	return Score;
}

