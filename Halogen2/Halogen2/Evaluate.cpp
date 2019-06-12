#include "Evaluate.h"

const unsigned int PieceValues[N_PIECES] = { 100, 320, 330, 500, 900, 10000, 100, 320, 330, 500, 900, 10000 };

const int PassedPawnBonus = 25;
const int WeakPawnPenalty = 10;
const int WeakOpenPawnPenalty = 20;
const int DoubledPawnPenalty = 10;

const int CastledBonus = 40;
const unsigned int Threat[N_PIECES] = { 1, 2, 2, 3, 5, 0, 1, 2, 2, 3, 5, 0 };

unsigned int CalculateGameStage(const Position& position);
int EvaluateCastleBonus(const Position& position);
int EvaluatePawn(const Position& position, unsigned int square, bool colour);
int EvaluatePawnStructure(const Position& position);
int EvaluatePieceSquareTables(const Position& position, unsigned int gameStage);
int EvaluateMaterial(const Position& position);
int EvaluateSaftey(Position& position);

const uint64_t CenterSquares = 0x1818000000;		
const uint64_t OuterSquares = 0x7e424242427e00;
const uint64_t InnerSquares = 0x3c24243c0000;

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

int EvaluatePosition(Position & position)
{
	int Score = 0;
	unsigned int GameStage = CalculateGameStage(position);

	int Material = EvaluateMaterial(position);
	int PieceSquares = EvaluatePieceSquareTables(position, GameStage);
	int PawnStructure = EvaluatePawnStructure(position);
	int Castle = EvaluateCastleBonus(position);
	int Control = EvaluateSaftey(position);
	Score += Material + PieceSquares + PawnStructure + Castle + Control;

	//std::cout << "Material: " << Material << "\n";
	//std::cout << "Piece Squares: " << PieceSquares << "\n";
	//std::cout << "Pawn Structure: " << PawnStructure << "\n";
	//std::cout << "Castle Bonus: " << Castle << "\n";
	//std::cout << "Total: " << Score << "\n";

	return Score;
}

void InitializeEvaluation()
{
	InitializePieceSquareTable();
}

unsigned int CalculateGameStage(const Position & position)
{
	//End game is defined as having only at most 3 pieces besides the king and pawns on both sides
	if ((GetBitCount(position.GetPieceBB(WHITE_ROOK) | position.GetPieceBB(WHITE_BISHOP) | position.GetPieceBB(WHITE_KNIGHT) | position.GetPieceBB(WHITE_QUEEN)) <= 3) && 
		(GetBitCount(position.GetPieceBB(BLACK_ROOK) | position.GetPieceBB(BLACK_BISHOP) | position.GetPieceBB(BLACK_KNIGHT) | position.GetPieceBB(BLACK_QUEEN)) <= 3))
		return ENDGAME;

	if (position.GetTurnCount() < 10)
		return OPENING;

	return MIDGAME;
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

int EvaluateSaftey(Position & position)
{
	//position.GenerateAttackTables();

	/*int AttacksOnWhite = 0;
	int AttacksOnBlack = 0;

	uint64_t BlackKingZone = WhiteKingAttackSquares[position.GetKing(position.GetTurn())];
	uint64_t WhiteKingZone = BlackKingAttackSquares[position.GetKing(position.GetTurn())];

	AttacksOnBlack += GetBitCount(BlackKingZone & position.GetAttackTable(WHITE_QUEEN)) * 5;
	AttacksOnBlack += GetBitCount(BlackKingZone & position.GetAttackTable(WHITE_ROOK)) * 3;
	AttacksOnBlack += GetBitCount(BlackKingZone & position.GetAttackTable(WHITE_KNIGHT)) * 2;
	AttacksOnBlack += GetBitCount(BlackKingZone & position.GetAttackTable(WHITE_BISHOP)) * 2;

	AttacksOnWhite += GetBitCount(WhiteKingZone & position.GetAttackTable(BLACK_QUEEN)) * 5;
	AttacksOnWhite += GetBitCount(WhiteKingZone & position.GetAttackTable(BLACK_ROOK)) * 3;
	AttacksOnWhite += GetBitCount(WhiteKingZone & position.GetAttackTable(BLACK_KNIGHT)) * 2;
	AttacksOnWhite += GetBitCount(WhiteKingZone & position.GetAttackTable(BLACK_BISHOP)) * 2;

	return (SafetyTable[AttacksOnBlack] - SafetyTable[AttacksOnWhite]) / 5;*/

	return 0;
}

