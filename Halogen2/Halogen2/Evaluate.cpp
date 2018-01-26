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
int EvaluateControl(const Position& position);

const uint64_t CenterSquares = 0x1818000000;		
const uint64_t OuterSquares = 0x7e424242427e00;
const uint64_t InnerSquares = 0x3c24243c0000;

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
	int Control = EvaluateControl(position);
	Score += Material + PieceSquares + PawnStructure + Castle + Control;

	/*std::cout << "Material: " << Material << "\n";
	std::cout << "Piece Squares: " << PieceSquares << "\n";
	std::cout << "Pawn Structure: " << PawnStructure << "\n";
	std::cout << "Castle Bonus: " << Castle << "\n";
	std::cout << "Total: " << Score << "\n";*/

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

int EvaluateControl(const Position & position)
{
	int WhiteCenterControl = GetBitCount((position.GetWhiteThreats() | position.GetWhitePieces()) & CenterSquares);
	int BlackCenterControl = GetBitCount((position.GetBlackThreats()| position.GetBlackPieces()) & CenterSquares);
	int WhiteInnerBoardControl = GetBitCount(position.GetWhiteThreats() & InnerSquares);
	int BlackInnerBoardControl = GetBitCount(position.GetBlackThreats() & InnerSquares);
	int WhiteOuterBoardControl = GetBitCount(position.GetWhiteThreats() & OuterSquares);
	int BlackOuterBoardControl = GetBitCount(position.GetBlackThreats() & OuterSquares);

	return (WhiteCenterControl - BlackCenterControl) * 4 + (WhiteInnerBoardControl - BlackInnerBoardControl) * 2 + (WhiteOuterBoardControl - BlackOuterBoardControl);
}

