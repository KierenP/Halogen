#include "Evaluate.h"

int pieceValueVector[N_STAGES][N_PIECE_TYPES] = { {91, 532, 568, 715, 1279, 5000},
												  {111, 339, 372, 638, 1301, 5000} };

int knightAdj[9] = {-77, -14, -4, 2, 8, 11, 17, 23, 30};	//adjustment of piece value based on the number of own pawns
int rookAdj[9] = {-6, -8, -3, -2, -4, -1, 1, 6, 14};

int WeakPawnPenalty = 4;
int WeakOpenPawnPenalty = 17;
int DoubledPawnPenalty = 14;

int PassedPawnBonus[N_RANKS] = {0, -12, -11, 11, 39, 148, 0, 0};	//final step passed pawns handled by PST as all final step pawns are passed

int CanCastleBonus = 29;
int CastledBonus = CanCastleBonus * 2;
int BishopPairBonus = 42;
int RookOpenFileBonus = 21;
int RookSemiOpenFileBonus = 24;

int TempoBonus = 22;

int KnightMobility = 8;
int KnightAverageMobility = 6;

int BishopMobility = 5;
int BishopAverageMobility = 7;

int RookMobility = 4;
int RookAverageMobility = 7;

int QueenMobility = 3;
int QueenAverageMobility = RookAverageMobility + BishopAverageMobility;

int EvaluateCastleBonus(const Position& position);
int EvaluatePawn(const Position& position, unsigned int square, bool colour);
int EvaluatePawnStructure(const Position& position);
int EvaluatePieceSquareTables(const Position& position, unsigned int gameStage);
int EvaluateMaterial(const Position& position, GameStages GameStage);
int KingTropism(const Position& pos);
int EvaluatePawns(const Position& position, GameStages gameStage);
int CalculateGamePhase(const Position& position);
int AdjustKnightScore(const Position& position);
int AdjustRookScore(const Position& position);
int RookFileAdjustment(const Position& position);
int CalculateTempo(const Position& position);
int Mobility(const Position& position);

bool InsufficentMaterialEvaluation(const Position& position);

bool BlackBlockade(uint64_t wPawns, uint64_t bPawns);
bool WhiteBlockade(uint64_t wPawns, uint64_t bPawns);

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

int PieceValues(unsigned int Piece, GameStages GameStage)
{
	return pieceValueVector[GameStage][Piece % N_PIECE_TYPES];
}

int EvaluatePosition(const Position & position)
{
	if (InsufficentMaterialEvaluation(position)) return 0;	

	int MidGame = 0;
	int EndGame = 0;
	int GamePhase = CalculateGamePhase(position);

	//Stuff independant of gameStage goes here:
	int Castle = EvaluateCastleBonus(position);
	int Tropism = KingTropism(position);

	int BishopPair = 0;

	if (GetBitCount(position.GetPieceBB(WHITE_BISHOP)) >= 2)
		BishopPair += BishopPairBonus;
	if (GetBitCount(position.GetPieceBB(BLACK_BISHOP)) >= 2)
		BishopPair -= BishopPairBonus;

	int KnightAdj = AdjustKnightScore(position);
	int RookAdj = AdjustRookScore(position);
	int RookFiles = RookFileAdjustment(position);

	int tempo = CalculateTempo(position);
	int mobility = Mobility(position);

	//stuff dependant on the game stage is calculated once for each
	int PieceSquaresMid = EvaluatePieceSquareTables(position, MIDGAME);
	int PieceSquaresEnd = EvaluatePieceSquareTables(position, ENDGAME);

	int pawnsMid = 0;
	int pawnsEnd = 0;
	QueryPawnHashTable(position, pawnsMid, MIDGAME);
	QueryPawnHashTable(position, pawnsEnd, ENDGAME);

	int MaterialMid = EvaluateMaterial(position, MIDGAME);
	int MaterialEnd = EvaluateMaterial(position, ENDGAME);

	MidGame = MaterialMid + PieceSquaresMid + Castle + Tropism + pawnsMid + BishopPair + KnightAdj + RookAdj + RookFiles + tempo + mobility;
	EndGame = MaterialEnd + PieceSquaresEnd + Castle + Tropism + pawnsEnd + BishopPair + KnightAdj + RookAdj + RookFiles + tempo + mobility;

	return ((MidGame * (256 - GamePhase)) + (EndGame * GamePhase)) / 256;
}

int Mobility(const Position& position)
{
	int Score = 0;
	uint64_t WhitePawnThreats = 0;
	uint64_t BlackPawnThreats = 0;

	WhitePawnThreats |= ((position.GetPieceBB(WHITE_PAWN) & ~(FileBB[FILE_A])) << 7);
	WhitePawnThreats |= ((position.GetPieceBB(WHITE_PAWN) & ~(FileBB[FILE_H])) << 9);

	BlackPawnThreats |= ((position.GetPieceBB(BLACK_PAWN) & ~(FileBB[FILE_A])) >> 9);				
	BlackPawnThreats |= ((position.GetPieceBB(BLACK_PAWN) & ~(FileBB[FILE_H])) >> 7);

	uint64_t pieces = position.GetAllPieces();

	for (uint64_t piece = position.GetPieceBB(WHITE_KNIGHT); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		unsigned int threats = GetBitCount(KnightAttacks[sq] & ~(BlackPawnThreats));
		Score += (threats - KnightAverageMobility) * KnightMobility;
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_KNIGHT); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		unsigned int threats = GetBitCount(KnightAttacks[sq] & ~(WhitePawnThreats));
		Score -= (threats - KnightAverageMobility) * KnightMobility;
	}

	for (uint64_t piece = position.GetPieceBB(WHITE_BISHOP); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = BishopAttacks[sq] & ~(BlackPawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score += (count - BishopAverageMobility) * BishopMobility;
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_BISHOP); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = BishopAttacks[sq] & ~(WhitePawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score -= (count - BishopAverageMobility) * BishopMobility;
	}

	for (uint64_t piece = position.GetPieceBB(WHITE_ROOK); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = RookAttacks[sq] & ~(BlackPawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score += (count - RookAverageMobility) * RookMobility;
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_ROOK); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = RookAttacks[sq] & ~(WhitePawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score -= (count - RookAverageMobility) * RookMobility;
	}

	for (uint64_t piece = position.GetPieceBB(WHITE_QUEEN); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = QueenAttacks[sq] & ~(BlackPawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score += (count - QueenAverageMobility) * QueenMobility;
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_QUEEN); piece != 0; )
	{
		unsigned int sq = bitScanForwardErase(piece);
		uint64_t targets = QueenAttacks[sq] & ~(WhitePawnThreats);
		unsigned int count = 0;

		while (targets != 0)
		{
			unsigned int target = bitScanForwardErase(targets);
			if (mayMove(sq, target, pieces))
				count++;
		}

		Score -= (count - QueenAverageMobility) * QueenMobility;
	}

	return Score;
}

int CalculateTempo(const Position& position)
{
	if (position.GetTurn() == WHITE)
		return TempoBonus;
	else
		return -TempoBonus;
}

int RookFileAdjustment(const Position& position)
{
	int Score = 0;
	for (uint64_t piece = position.GetPieceBB(WHITE_ROOK); piece != 0; )
	{
		int sq = bitScanForwardErase(piece);	
		int file = GetFile(sq);

		if ((GetBitCount(position.GetPieceBB(WHITE_PAWN) & FileBB[file])) == 0)
		{
			if ((GetBitCount(position.GetPieceBB(BLACK_PAWN) & FileBB[file])) == 0)	
			{
				Score += RookOpenFileBonus;
			}
			else 
			{
				Score += RookSemiOpenFileBonus;
			}
		}
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_ROOK); piece != 0; )
	{
		int sq = bitScanForwardErase(piece);
		int file = GetFile(sq);

		if ((GetBitCount(position.GetPieceBB(BLACK_PAWN) & FileBB[file])) == 0)
		{
			if ((GetBitCount(position.GetPieceBB(WHITE_PAWN) & FileBB[file])) == 0)
			{
				Score -= RookOpenFileBonus;
			}
			else
			{
				Score -= RookSemiOpenFileBonus;
			}
		}
	}

	return Score;
}

int AdjustKnightScore(const Position& position)
{
	assert(GetBitCount(position.GetPieceBB(WHITE_PAWN)) <= 8);
	assert(GetBitCount(position.GetPieceBB(BLACK_PAWN)) <= 8);

	int Score = 0;
	Score += knightAdj[GetBitCount(position.GetPieceBB(WHITE_PAWN))] * GetBitCount(position.GetPieceBB(WHITE_KNIGHT));
	Score -= knightAdj[GetBitCount(position.GetPieceBB(BLACK_PAWN))] * GetBitCount(position.GetPieceBB(BLACK_KNIGHT));
	return Score;
}

int AdjustRookScore(const Position& position)
{
	assert(GetBitCount(position.GetPieceBB(WHITE_PAWN)) <= 8);
	assert(GetBitCount(position.GetPieceBB(BLACK_PAWN)) <= 8);

	int Score = 0;
	Score += rookAdj[GetBitCount(position.GetPieceBB(WHITE_PAWN))] * GetBitCount(position.GetPieceBB(WHITE_ROOK));
	Score -= rookAdj[GetBitCount(position.GetPieceBB(BLACK_PAWN))] * GetBitCount(position.GetPieceBB(BLACK_ROOK));

	return Score;
}

int CalculateGamePhase(const Position& position)
{
	int PieceWeightings[N_PIECE_TYPES] = { 0, 1, 1, 2, 4, 0 };
	int	TotalPhase = 24;

	int	phase = TotalPhase;

	for (int i = PAWN; i < N_PIECE_TYPES; i++)
	{
		phase -= GetBitCount(position.GetPieceBB(i, WHITE)) * PieceWeightings[i];
		phase -= GetBitCount(position.GetPieceBB(i, BLACK)) * PieceWeightings[i];
	}

	phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

	//assert(phase >= 0);
	//assert(phase <= 256);	//promotions can cause values outside of this range, but I think thats ok

	return phase;
}

void QueryPawnHashTable(const Position& position, int& pawns, GameStages GameStage)
{
	uint64_t pawnKey = PawnHashKey(position, GameStage);

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
}

int EvaluatePawns(const Position& position, GameStages gameStage)
{
	int Score = 0;

	Score -= GetBitCount(position.GetPieceBB(BLACK_PAWN)) * PieceValues(BLACK_PAWN, gameStage);		//material																												
	Score += GetBitCount(position.GetPieceBB(WHITE_PAWN)) * PieceValues(WHITE_PAWN, gameStage);

	for (uint64_t piece = position.GetPieceBB(BLACK_PAWN); piece != 0; Score -= PieceSquareTables[gameStage][BLACK_PAWN][bitScanForwardErase(piece)]);						//piece square tables
	for (uint64_t piece = position.GetPieceBB(WHITE_PAWN); piece != 0; Score += PieceSquareTables[gameStage][WHITE_PAWN][bitScanForwardErase(piece)]);				

	Score += EvaluatePawnStructure(position);

	return Score;
}

bool InsufficentMaterialEvaluation(const Position& position)
{
	if ((position.GetPieceBB(WHITE_PAWN)) != 0) return false;
	if ((position.GetPieceBB(WHITE_ROOK)) != 0) return false;
	if ((position.GetPieceBB(WHITE_QUEEN)) != 0) return false;

	if ((position.GetPieceBB(BLACK_PAWN)) != 0) return false;
	if ((position.GetPieceBB(BLACK_ROOK)) != 0) return false;
	if ((position.GetPieceBB(BLACK_QUEEN)) != 0) return false;

	/*
	From the Chess Programming Wiki:

		According to the rules of a dead position, Article 5.2 b, when there is no possibility of checkmate for either side with any series of legal moves, the position is an immediate draw if

		- both sides have a bare king																																									1.
		- one side has a king and a minor piece against a bare king																																		1.
		- both sides have a king and a bishop, the bishops being the same color																															1.
		
		The bishops of different colors are not counted as an immediate draw, because of the possibility of a helpmate in the corner. 
		Since this is unlikely given even a four ply search, we may introduce another class of drawn positions: those that cannot be claimed, but can be evaluated as draws:

		- two knights against the bare king																																								2.
		- both sides have a king and a minor piece each																																					1.
		- the weaker side has a minor piece against two knights																																			2.
		- two bishops draw against a bishop																																								4.
		- two minor pieces against one draw, except when the stronger side has a bishop pair																											3.

		Please note that a knight or even two knights against two bishops are not included here, as it is possible to win this ending.
	*/

	//We know the board must contain just knights, bishops and kings
	int WhiteBishops = GetBitCount(position.GetPieceBB(WHITE_BISHOP));
	int BlackBishops = GetBitCount(position.GetPieceBB(BLACK_BISHOP));
	int WhiteKnights = GetBitCount(position.GetPieceBB(WHITE_KNIGHT));
	int BlackKnights = GetBitCount(position.GetPieceBB(BLACK_KNIGHT));
	int WhiteMinor = WhiteBishops + WhiteKnights;
	int BlackMinor = BlackBishops + BlackKnights;

	if (WhiteMinor <= 1 && BlackMinor <= 1) return true;												//1		
	if (WhiteMinor <= 1 && BlackKnights <= 2 && BlackBishops == 0) return true;							//2		KNvKNN, KBvKNN, KvKNN or combinations with less nights
	if (BlackMinor <= 1 && WhiteKnights <= 2 && WhiteBishops == 0) return true;							//2		
	if (WhiteMinor <= 1 && BlackMinor <= 2 && BlackBishops < 2) return true;							//3		
	if (BlackMinor <= 1 && WhiteMinor <= 2 && WhiteBishops < 2) return true;							//3
	if (WhiteBishops == 1 && BlackBishops == 2 && WhiteKnights == 0 && BlackKnights == 0) return true;	//4
	if (BlackBishops == 1 && WhiteBishops == 2 && BlackKnights == 0 && WhiteKnights == 0) return true;	//4

	return false;
}

void EvalInit()
{
	InitializePieceSquareTable();

	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			Distance[i][j] = 14 - (AbsFileDiff(i, j) + AbsRankDiff(i, j));
		}
	}
}

bool DeadPosition(const Position& position)
{
	if ((position.GetPieceBB(WHITE_PAWN)) != 0) return false;
	if ((position.GetPieceBB(WHITE_ROOK)) != 0) return false;
	if ((position.GetPieceBB(WHITE_QUEEN)) != 0) return false;

	if ((position.GetPieceBB(BLACK_PAWN)) != 0) return false;
	if ((position.GetPieceBB(BLACK_ROOK)) != 0) return false;
	if ((position.GetPieceBB(BLACK_QUEEN)) != 0) return false;

	/*
	From the Chess Programming Wiki:

		According to the rules of a dead position, Article 5.2 b, when there is no possibility of checkmate for either side with any series of legal moves, the position is an immediate draw if

		- both sides have a bare king													1.																																						
		- one side has a king and a minor piece against a bare king						2.																																
		- both sides have a king and a bishop, the bishops being the same color			Not covered																												
	*/

	//We know the board must contain just knights, bishops and kings
	int WhiteBishops = GetBitCount(position.GetPieceBB(WHITE_BISHOP));
	int BlackBishops = GetBitCount(position.GetPieceBB(BLACK_BISHOP));
	int WhiteKnights = GetBitCount(position.GetPieceBB(WHITE_KNIGHT));
	int BlackKnights = GetBitCount(position.GetPieceBB(BLACK_KNIGHT));
	int WhiteMinor = WhiteBishops + WhiteKnights;
	int BlackMinor = BlackBishops + BlackKnights;

	if (WhiteMinor == 0 && BlackMinor == 0) return true;	//1
	if (WhiteMinor == 1 && BlackMinor == 0) return true;	//2
	if (WhiteMinor == 0 && BlackMinor == 1) return true;	//2

	return false;
}

bool IsBlockade(const Position& position)
{
	if (position.GetAllPieces() != (position.GetPieceBB(WHITE_KING) | position.GetPieceBB(BLACK_KING) | position.GetPieceBB(WHITE_PAWN) | position.GetPieceBB(BLACK_PAWN)))
		return false;

	if (position.GetTurn() == WHITE)
		return BlackBlockade(position.GetPieceBB(WHITE_PAWN), position.GetPieceBB(BLACK_PAWN));
	else
		return WhiteBlockade(position.GetPieceBB(WHITE_PAWN), position.GetPieceBB(BLACK_PAWN));
}

/*addapted from chess programming wiki code example*/
bool BlackBlockade(uint64_t wPawns, uint64_t bPawns) 
{
	uint64_t fence = wPawns & (bPawns >> 8);					//blocked white pawns
	if (GetBitCount(fence) < 3)
		return false;

	fence |= ((bPawns & ~(FileBB[FILE_A])) >> 9);				//black pawn attacks
	fence |= ((bPawns & ~(FileBB[FILE_H])) >> 7);

	uint64_t flood = RankBB[RANK_1] | allBitsBelow[bitScanForward(fence)];
	uint64_t above = allBitsAbove[bitScanReverse(fence)];

	if ((wPawns & ~fence) != 0)
		return false;

	while (true) {
		uint64_t temp = flood;
		flood |= ((flood & ~(FileBB[FILE_A])) >> 1) | ((flood & ~(FileBB[FILE_H])) << 1);	//Add left and right 
		flood |= (flood << 8) | (flood >> 8);												//up and down
		flood &= ~fence;

		if (flood == temp)
			break; /* Fill has stopped, blockage? */
		if (flood & above) /* break through? */
			return false; /* yes, no blockage */
		if (flood & bPawns) {
			bPawns &= ~flood;  /* "capture" undefended black pawns */
			fence = wPawns & (bPawns >> 8);
			if (GetBitCount(fence) < 3)
				return false;

			fence |= ((bPawns & ~(FileBB[FILE_A])) >> 9);				
			fence |= ((bPawns & ~(FileBB[FILE_H])) >> 7);
		}
	}

	return true;
}

bool WhiteBlockade(uint64_t wPawns, uint64_t bPawns)
{
	uint64_t fence = bPawns & (wPawns << 8);					
	if (GetBitCount(fence) < 3)
		return false;

	fence |= ((wPawns & ~(FileBB[FILE_A])) << 7);
	fence |= ((wPawns & ~(FileBB[FILE_H])) << 9);

	uint64_t flood = RankBB[RANK_8] | allBitsAbove[bitScanReverse(fence)];
	uint64_t above = allBitsBelow[bitScanForward(fence)];

	if ((bPawns & ~fence) != 0)
		return false;

	while (true) {
		uint64_t temp = flood;
		flood |= ((flood & ~(FileBB[FILE_A])) >> 1) | ((flood & ~(FileBB[FILE_H])) << 1);	//Add left and right 
		flood |= (flood << 8) | (flood >> 8);												//up and down
		flood &= ~fence;

		if (flood == temp)
			break; /* Fill has stopped, blockage? */
		if (flood & above) /* break through? */
			return false; /* yes, no blockage */
		if (flood & bPawns) {
			bPawns &= ~flood;  /* "capture" undefended black pawns */
			fence = bPawns & (wPawns << 8);
			if (GetBitCount(fence) < 3)
				return false;

			fence |= ((wPawns & ~(FileBB[FILE_A])) << 7);
			fence |= ((wPawns & ~(FileBB[FILE_H])) << 9);
		}
	}
	return true;
}

int EvaluateCastleBonus(const Position & position)
{
	int score = 0;

	if (position.HasCastledWhite())
		score += CastledBonus;

	if (position.HasCastledBlack())
		score -= CastledBonus;

	if (position.CanCastleWhiteKingside() || position.CanCastleWhiteQueenside())
		score += CanCastleBonus;

	if (position.CanCastleBlackKingside() || position.CanCastleBlackQueenside())
		score -= CanCastleBonus;

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

int EvaluateMaterial(const Position & position, GameStages GameStage)
{
	int Score = 0;

	for (int i = 1; i < N_PIECE_TYPES; i++)	//skip pawn
	{
		Score -= GetBitCount(position.GetPieceBB(i)) * PieceValues(i, GameStage);																														//black piece
		Score += GetBitCount(position.GetPieceBB(i + N_PIECE_TYPES)) * PieceValues(i + N_PIECE_TYPES, GameStage);																						//white piece										
	}

	return Score;
}

bool EvaluateDebug()
{
#ifdef NDEBUG
	return true;
#else
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

			testPosition.FlipColours();
			testPosition.MirrorTopBottom();
			assert(score == -EvaluatePosition(testPosition));
			testPosition.FlipColours();
			testPosition.MirrorTopBottom();
			

			for (int j = 0; j < N_SQUARES; j++)
			{
				assert(testPosition.GetSquare(j) == copy.GetSquare(j));
			}
		}
	}

	return true;
#endif
}

std::vector<int*> TexelParamiters()
{
	std::vector<int*> params;

	params.push_back(&pieceValueVector[MIDGAME][0]);
	params.push_back(&pieceValueVector[MIDGAME][1]);
	params.push_back(&pieceValueVector[MIDGAME][2]);
	params.push_back(&pieceValueVector[MIDGAME][3]);
	params.push_back(&pieceValueVector[MIDGAME][4]);
	params.push_back(&pieceValueVector[ENDGAME][0]);
	params.push_back(&pieceValueVector[ENDGAME][1]);
	params.push_back(&pieceValueVector[ENDGAME][2]);
	params.push_back(&pieceValueVector[ENDGAME][3]);
	params.push_back(&pieceValueVector[ENDGAME][4]);
	params.push_back(&WeakPawnPenalty);
	params.push_back(&WeakOpenPawnPenalty);
	params.push_back(&DoubledPawnPenalty);
	params.push_back(&BishopPairBonus);
	params.push_back(&RookOpenFileBonus);
	params.push_back(&RookSemiOpenFileBonus);
	params.push_back(&TempoBonus);
	params.push_back(&CanCastleBonus);

	for (int i = 0; i < 9; i++)
	{
		params.push_back(&knightAdj[i]);
	}

	for (int i = 0; i < 9; i++)
	{
		params.push_back(&rookAdj[i]);
	}

	for (int i = 1; i < 6; i++)	
	{
		params.push_back(&PassedPawnBonus[i]);
	}

	params.push_back(&KnightMobility);
	params.push_back(&BishopMobility);
	params.push_back(&RookMobility);
	params.push_back(&QueenMobility);

	return params;
}

std::vector<int*> TexelPST()
{
	std::vector<int*> PST;

	PST.push_back(PawnSquareValuesMid);
	PST.push_back(PawnSquareValuesEndGame);
	PST.push_back(KnightSquareValues);
	PST.push_back(BishopSquareValues);
	PST.push_back(RookSquareValues);
	PST.push_back(QueenSquareValues);
	PST.push_back(KingSquareMid);
	PST.push_back(KingSquareEndGame);

	return PST;
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



