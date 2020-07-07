#include "Evaluate.h"

int pieceValueVector[N_PIECE_TYPES] = {95, 337, 330, 545, 1024, 5000};

int knightAdj[9] = {-129, -53, -38, -31, -25, -20, -16, -9, 2};	//adjustment of piece value based on the number of own pawns
int rookAdj[9] = {-39, -33, -29, -30, -35, -38, -43, -49, -54};

int WeakPawnPenalty = 5;
int WeakOpenPawnPenalty = 15;
int DoubledPawnPenalty = 9;

int PassedPawnBonus[N_RANKS] = {0, 1, -2, 9, 32, 88, 143, 0};

int CastledBonus = 40;
int BishopPairBonus = 32;
int RookOpenFileBonus = 30;
int RookSemiOpenFileBonus = 23;

int TempoBonus = 17;

int EvaluateCastleBonus(const Position& position);
int EvaluatePawn(const Position& position, unsigned int square, bool colour);
int EvaluatePawnStructure(const Position& position);
int EvaluatePieceSquareTables(const Position& position, unsigned int gameStage);
int EvaluateMaterial(const Position& position);
int KingTropism(const Position& pos);
int EvaluatePawns(const Position& position, unsigned int gameStage);
int CalculateGamePhase(const Position& position);
int AdjustKnightScore(const Position& position);
int AdjustRookScore(const Position& position);
int RookFileAdjustment(const Position& position);
int CalculateTempo(const Position& position);

bool InsufficentMaterialEvaluation(const Position& position, int Material);		//if you already have the material score -> faster!
bool InsufficentMaterialEvaluation(const Position& position);					//will count the material for you

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

//DEBUG FUNCTIONS
void FlipColours(Position& pos);
void MirrorTopBottom(Position& pos);
void MirrorLeftRight(Position& pos);

int PieceValues(unsigned int Piece)
{
	if (Piece < N_PIECE_TYPES)
		return pieceValueVector[Piece];
	else
		return pieceValueVector[Piece - N_PIECE_TYPES];
}

int EvaluatePosition(const Position & position)
{
	int Material = EvaluateMaterial(position);	
	if (InsufficentMaterialEvaluation(position, Material)) return 0;	//note the Material doesn't include the pawns, but if there were any pawns we return false anyways so that doesn't matter

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

	//stuff dependant on the game stage is calculated once for each
	int PieceSquaresMid = EvaluatePieceSquareTables(position, MIDGAME);
	int PieceSquaresEnd = EvaluatePieceSquareTables(position, ENDGAME);

	int pawnsMid = 0;
	int pawnsEnd = 0;
	QueryPawnHashTable(position, pawnsMid, MIDGAME);
	QueryPawnHashTable(position, pawnsEnd, ENDGAME);

	MidGame = Material + PieceSquaresMid + Castle + Tropism + pawnsMid + BishopPair + KnightAdj + RookAdj + RookFiles + tempo;
	EndGame = Material + PieceSquaresEnd + Castle + Tropism + pawnsEnd + BishopPair + KnightAdj + RookAdj + RookFiles + tempo;

	return ((MidGame * (256 - GamePhase)) + (EndGame * GamePhase)) / 256;
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
	int Score = 0;
	for (uint64_t piece = position.GetPieceBB(WHITE_KNIGHT); piece != 0; )
	{
		bitScanForwardErase(piece);	//clear the LSB
		Score += knightAdj[GetBitCount(position.GetPieceBB(WHITE_PAWN))];
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_KNIGHT); piece != 0; )
	{
		bitScanForwardErase(piece);	//clear the LSB
		Score -= knightAdj[GetBitCount(position.GetPieceBB(BLACK_PAWN))];
	}
	return Score;
}

int AdjustRookScore(const Position& position)
{
	assert(GetBitCount(position.GetPieceBB(WHITE_PAWN)) <= 8);
	assert(GetBitCount(position.GetPieceBB(BLACK_PAWN)) <= 8);

	int Score = 0;
	for (uint64_t piece = position.GetPieceBB(WHITE_ROOK); piece != 0; )
	{
		bitScanForwardErase(piece);	//clear the LSB
		Score += rookAdj[GetBitCount(position.GetPieceBB(WHITE_PAWN))];
	}

	for (uint64_t piece = position.GetPieceBB(BLACK_ROOK); piece != 0; )
	{
		bitScanForwardErase(piece);	//clear the LSB
		Score -= rookAdj[GetBitCount(position.GetPieceBB(BLACK_PAWN))];
	}
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

void QueryPawnHashTable(const Position& position, int& pawns, unsigned int GameStage)
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

int EvaluatePawns(const Position& position, unsigned int gameStage)
{
	int Score = 0;

	Score -= GetBitCount(position.GetPieceBB(BLACK_PAWN)) * PieceValues(BLACK_PAWN);		//material																												
	Score += GetBitCount(position.GetPieceBB(WHITE_PAWN)) * PieceValues(WHITE_PAWN);	

	for (uint64_t piece = position.GetPieceBB(BLACK_PAWN); piece != 0; Score -= PieceSquareTables[gameStage][BLACK_PAWN][bitScanForwardErase(piece)]);						//piece square tables
	for (uint64_t piece = position.GetPieceBB(WHITE_PAWN); piece != 0; Score += PieceSquareTables[gameStage][WHITE_PAWN][bitScanForwardErase(piece)]);				

	Score += EvaluatePawnStructure(position);

	return Score;
}

bool InsufficentMaterialEvaluation(const Position& position, int Material)
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

bool InsufficentMaterialEvaluation(const Position& position)
{
	int Material = EvaluateMaterial(position);
	return InsufficentMaterialEvaluation(position, Material);
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
	if (position.GetTurn() == BLACK)
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
		Score -= GetBitCount(position.GetPieceBB(i)) * PieceValues(i);																														//black piece
		Score += GetBitCount(position.GetPieceBB(i + N_PIECE_TYPES)) * PieceValues(i + N_PIECE_TYPES);																						//white piece										
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

			for (int j = 0; j < N_SQUARES; j++)
			{
				assert(testPosition.GetSquare(j) == copy.GetSquare(j));
			}
		}
	}

	return true;
}

std::vector<int*> TexelParamiters()
{
	std::vector<int*> params { 
			&pieceValueVector[0], 
			&pieceValueVector[1], 
			&pieceValueVector[2], 
			&pieceValueVector[3], 
			&pieceValueVector[4], 
			&WeakPawnPenalty,
			&WeakOpenPawnPenalty,
			&DoubledPawnPenalty,
			&BishopPairBonus,
			&RookOpenFileBonus,
			&RookSemiOpenFileBonus,
			&TempoBonus	};

	for (int i = 0; i < 9; i++)
	{
		params.push_back(&knightAdj[i]);
	}

	for (int i = 0; i < 9; i++)
	{
		params.push_back(&rookAdj[i]);
	}

	for (int i = 1; i <= 6; i++)
	{
		params.push_back(&PassedPawnBonus[i]);
	}

	return params;
}

void FlipColours(Position& pos)
{
	pos.ApplyNullMove();

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



