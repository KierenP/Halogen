#include "Evaluate.h"

const unsigned int PieceValues[N_PIECES] = { 100, 320, 330, 500, 900, 5000, 100, 320, 330, 500, 900, 5000 };

const int WeakPawnPenalty = 10;
const int WeakOpenPawnPenalty = 20;
const int DoubledPawnPenalty = 10;

const int PassedPawnBonus[N_RANKS] = { 0, 10, 20, 30, 60, 120, 150, 0 };

const int CastledBonus = 40;

unsigned int CalculateGameStage(const Position& position);
int EvaluateCastleBonus(const Position& position);
int EvaluatePawn(const Position& position, unsigned int square, bool colour);
int EvaluatePawnStructure(const Position& position);
int EvaluatePieceSquareTables(const Position& position, unsigned int gameStage);
int EvaluateMaterial(const Position& position);
int KingTropism(const Position& pos);
int EvaluatePawns(const Position& position, unsigned int gameStage);

PawnHashTable pawnHashTable;

int Distance[64][64];

int Diagonals[64] = {
   0, 1, 2, 3, 4, 5, 6, 7,
   1, 2, 3, 4, 5, 6, 7, 8,
   2, 3, 4, 5, 6, 7, 8, 9,
   3, 4, 5, 6, 7, 8, 9,10,
   4, 5, 6, 7, 8, 9,10,11,
   5, 6, 7, 8, 9,10,11,12,
   6, 7, 8, 9,10,11,12,13,
   7, 8, 9,10,11,12,13,14
};

int AntiDiagonals[64] = {
   7, 6, 5, 4, 3, 2, 1, 0,
   8, 7, 6, 5, 4, 3, 2, 1,
   9, 8, 7, 6, 5, 4, 3, 2,
  10, 9, 8, 7, 6, 5, 4, 3,
  11,10, 9, 8, 7, 6, 5, 4,
  12,11,10, 9, 8, 7, 6, 5,
  13,12,11,10, 9, 8, 7, 6,
  14,13,12,11,10, 9, 8, 7
};

//DEBUG FUNCTIONS
void FlipColours(Position& pos);
void MirrorTopBottom(Position& pos);
void MirrorLeftRight(Position& pos);

int EvaluatePosition(const Position & position)
{
	int Score = 0;
	unsigned int GameStage = CalculateGameStage(position);

	int Material = EvaluateMaterial(position);	
	if (InsufficentMaterial(position, Material)) return 0;	//note the Material doesn't include the pawns, but if there were any pawns we return false anyways so that doesn't matter
	int PieceSquares = EvaluatePieceSquareTables(position, GameStage);
	int Castle = EvaluateCastleBonus(position);
	int Tropism = KingTropism(position);

	//int pawns = EvaluatePawns(position, GameStage);
	int pawns = 0;
	uint64_t pawnKey = PawnHashKey(position);

	if (pawnHashTable.CheckEntry(pawnKey))
	{
		pawnHashTable.HashHits++;
		pawns = pawnHashTable.GetEntry(pawnKey).eval;
	}
	else
	{
		pawnHashTable.HashMisses++;
		pawns = EvaluatePawns(position, GameStage);
		pawnHashTable.AddEntry(pawnKey, pawns);
	}

	Score += Material + PieceSquares + Castle + Tropism + pawns;
	return Score;
}

int EvaluatePawns(const Position& position, unsigned int gameStage)
{
	int Score = 0;

	Score -= GetBitCount(position.GetPieceBB(BLACK_PAWN)) * PieceValues[BLACK_PAWN];		//material																												
	Score += GetBitCount(position.GetPieceBB(WHITE_PAWN)) * PieceValues[WHITE_PAWN];	

	for (uint64_t piece = position.GetPieceBB(BLACK_PAWN); piece != 0; Score -= PieceSquareTables[gameStage][BLACK_PAWN][bitScanForwardErase(piece)]);						//piece square tables
	for (uint64_t piece = position.GetPieceBB(WHITE_PAWN); piece != 0; Score += PieceSquareTables[gameStage][WHITE_PAWN][bitScanForwardErase(piece)]);				

	Score += EvaluatePawnStructure(position);

	return Score;
}

bool InsufficentMaterial(const Position& position, int Material)
{
	if ((position.GetPieceBB(WHITE_PAWN)) != 0) return false;
	if ((position.GetPieceBB(WHITE_ROOK)) != 0) return false;
	if ((position.GetPieceBB(WHITE_QUEEN)) != 0) return false;

	if ((position.GetPieceBB(BLACK_PAWN)) != 0) return false;
	if ((position.GetPieceBB(BLACK_ROOK)) != 0) return false;
	if ((position.GetPieceBB(BLACK_QUEEN)) != 0) return false;

	//in almost all cases, you need to have more than 400 points difference. That equates to 2 or more minor pieces (KNN, KNB, KBB). 
	//Note KNN is not a forced win and should be counted as a draw, but 
	if (abs(Material) < 400) return true;	

	return false;
}

bool InsufficentMaterial(const Position& position)
{
	int Material = EvaluateMaterial(position);
	return InsufficentMaterial(position, Material);
}

void InitializeEvaluation()
{
	InitializePieceSquareTable();

	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			Distance[i][j] = 14 - (AbsFileDiff(i, j) + AbsRankDiff(i, j));
		}
	}
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
	assert(square < N_SQUARES);
	assert(GetRank(square) != RANK_1);
	assert(GetRank(square) != RANK_8);

	int result = 0;
	bool IsPassed = false;
	bool IsWeak = true;
	bool IsOpposed = false;
	bool IsDoubled = false;

	int back = 0;

	if (colour == WHITE)	{ back = -8; }
	else					{ back = 8; }

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
	{
		if (colour == WHITE)
			result += PassedPawnBonus[GetRank(square)];
		else
			result += PassedPawnBonus[RANK_8 - GetRank(square)];
	}

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

	for (uint64_t WhitePawns = position.GetPieceBB(WHITE_PAWN); WhitePawns != 0; result += EvaluatePawn(position, bitScanForwardErase(WhitePawns), WHITE));
	for (uint64_t BlackPawns = position.GetPieceBB(BLACK_PAWN); BlackPawns != 0; result -= EvaluatePawn(position, bitScanForwardErase(BlackPawns), BLACK));

	return result;
}

int EvaluatePieceSquareTables(const Position & position, unsigned int gameStage)
{
	int Score = 0;

	for (int i = 1; i < N_PIECE_TYPES; i++)	//skip pawn
	{
		for (uint64_t piece = position.GetPieceBB(i); piece != 0; Score -= PieceSquareTables[gameStage][i][bitScanForwardErase(piece)]);														//black piece
		for (uint64_t piece = position.GetPieceBB(i + N_PIECE_TYPES); piece != 0; Score += PieceSquareTables[gameStage][i + N_PIECE_TYPES][bitScanForwardErase(piece)]);						//white piece
	}

	return Score;
}

int EvaluateMaterial(const Position & position)
{
	int Score = 0;

	for (int i = 1; i < N_PIECE_TYPES; i++)	//skip pawn
	{
		Score -= GetBitCount(position.GetPieceBB(i)) * PieceValues[i];																														//black piece
		Score += GetBitCount(position.GetPieceBB(i + N_PIECE_TYPES)) * PieceValues[i + N_PIECE_TYPES];																						//white piece										
	}

	return Score;
}

bool EvaluateDebug()
{
	pawnHashTable.Init(1);

	for (int i = 0; i <= 1; i++)	//2 pass to check pawn eval hash on 2nd run
	{
		std::ifstream infile("perftsuite.txt");
		std::string line;

		while (std::getline(infile, line))
		{
			std::vector<std::string> arrayTokens;
			std::istringstream iss(line);
			arrayTokens.clear();

			do
			{
				std::string stub;
				iss >> stub;
				arrayTokens.push_back(stub);
			} while (iss);

			Position testPosition;
			testPosition.InitialiseFromFen(line);
			Position copy = testPosition;
			int score = EvaluatePosition(testPosition);

			FlipColours(testPosition);
			MirrorTopBottom(testPosition);

			assert(score == -EvaluatePosition(testPosition));

			MirrorLeftRight(testPosition);
			assert(score == -EvaluatePosition(testPosition));

			FlipColours(testPosition);
			MirrorTopBottom(testPosition);
			assert(score == EvaluatePosition(testPosition));

			MirrorLeftRight(testPosition);

			for (int i = 0; i < N_SQUARES; i++)
			{
				assert(testPosition.GetSquare(i) == copy.GetSquare(i));
			}
		}
	}

	return true;
}

void FlipColours(Position& pos)
{
	for (int i = 0; i < N_SQUARES; i++)
	{
		if (pos.IsOccupied(i))
			pos.SetSquare(i, Piece(pos.GetSquare(i) % N_PIECE_TYPES, !ColourOfPiece(pos.GetSquare(i))));
	}
}

void MirrorLeftRight(Position& pos)
{
	Position copy = pos;

	for (int i = 0; i < N_SQUARES; i++)
	{
		pos.ClearSquare(i);
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		if (copy.IsOccupied(GetPosition(N_FILES - GetFile(i) - 1, GetRank(i))))
			pos.SetSquare(i, copy.GetSquare(GetPosition(N_FILES - GetFile(i) - 1, GetRank(i))));
	}
}

void MirrorTopBottom(Position& pos)
{
	Position copy = pos;

	for (int i = 0; i < N_SQUARES; i++)
	{
		pos.ClearSquare(i);
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		if (copy.IsOccupied(GetPosition(GetFile(i), N_RANKS - GetRank(i) - 1)))
			pos.SetSquare(i, copy.GetSquare(GetPosition(GetFile(i), N_RANKS - GetRank(i) - 1)));
	}
}

//gives a score based on how close the pieces are to an opponents king
int KingTropism(const Position& pos)
{
	int WhiteTropism = 0;
	int BlackTropism = 0;
	int whiteKing = pos.GetKing(WHITE);
	int blackKing = pos.GetKing(BLACK);

	int bonus_dia_distance[15] = { 5, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (uint64_t pieces = pos.GetPieceBB(KNIGHT, WHITE); pieces != 0; BlackTropism += Distance[bitScanForwardErase(pieces)][blackKing]);
	for (uint64_t pieces = pos.GetPieceBB(KNIGHT, BLACK); pieces != 0; WhiteTropism += Distance[bitScanForwardErase(pieces)][whiteKing]);

	for (uint64_t pieces = pos.GetPieceBB(ROOK, WHITE); pieces != 0; BlackTropism += Distance[bitScanForwardErase(pieces)][blackKing] / 2);
	for (uint64_t pieces = pos.GetPieceBB(ROOK, BLACK); pieces != 0; WhiteTropism += Distance[bitScanForwardErase(pieces)][whiteKing] / 2);

	for (uint64_t pieces = pos.GetPieceBB(QUEEN, WHITE); pieces != 0; BlackTropism += Distance[bitScanForwardErase(pieces)][blackKing] * 5 / 2);
	for (uint64_t pieces = pos.GetPieceBB(QUEEN, BLACK); pieces != 0; WhiteTropism += Distance[bitScanForwardErase(pieces)][whiteKing] * 5 / 2);

	for (uint64_t pieces = pos.GetPieceBB(BISHOP, WHITE); pieces != 0; )
	{
		int sq = bitScanForwardErase(pieces);

		BlackTropism += bonus_dia_distance[abs(Diagonals[sq] - Diagonals[blackKing])];
		BlackTropism += bonus_dia_distance[abs(AntiDiagonals[sq] - AntiDiagonals[blackKing])];
	}
	
	for (uint64_t pieces = pos.GetPieceBB(BISHOP, BLACK); pieces != 0; )
	{
		int sq = bitScanForwardErase(pieces);

		WhiteTropism += bonus_dia_distance[abs(Diagonals[sq] - Diagonals[whiteKing])];
		WhiteTropism += bonus_dia_distance[abs(AntiDiagonals[sq] - AntiDiagonals[whiteKing])];
	}
	
	return BlackTropism - WhiteTropism;
}



