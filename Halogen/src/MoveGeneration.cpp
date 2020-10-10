#include "MoveGeneration.h"

void GenerateLegalMoves(Position& position, std::vector<Move>& moves, uint64_t pinned);
void AddQuiescenceMoves(Position& position, std::vector<Move>& moves, uint64_t pinned);	//captures and/or promotions

//Pawn moves
void PawnPushes(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnPromotions(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnDoublePushes(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnEnPassant(Position& position, std::vector<Move>& moves);	//Ep moves are always checked for legality so no need for pinned mask
void PawnCaptures(Position& position, std::vector<Move>& moves, uint64_t pinned);

//All other pieces
void GenerateQuietMoves(Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding, uint64_t pinned);
void GenerateCaptureMoves(Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding, uint64_t pinned);

//misc
void CastleMoves(const Position& position, std::vector<Move>& moves);

//utility functions
bool MovePutsSelfInCheck(Position& position, const Move& move);
uint64_t PinnedMask(const Position& position);

//special generators for when in check
void KingEvasions(Position& position, std::vector<Move>& moves);						//move the king out of danger	(single or multi threat)
void KingCapturesEvade(Position& position, std::vector<Move>& moves);			//use only for multi threat with king evasions
void CaptureThreat(Position& position, std::vector<Move>& moves, uint64_t threats);		//capture the attacker	(single threat only)
void BlockThreat(Position& position, std::vector<Move>& moves, uint64_t threats);		//block the attacker (single threat only)

void LegalMoves(Position& position, std::vector<Move>& moves)
{
	uint64_t pinned = PinnedMask(position);

	if (IsInCheck(position, position.GetTurn()))
	{
		moves.reserve(10);

		uint64_t Threats = GetThreats(position, position.GetKing(position.GetTurn()), position.GetTurn());
		assert(Threats != 0);

		if (GetBitCount(Threats) > 1)					//double check
		{
			KingEvasions(position, moves);
			KingCapturesEvade(position, moves);
		}
		else
		{
			PawnPushes(position, moves, pinned);		//pawn moves are hard :( so we calculate those normally
			PawnDoublePushes(position, moves, pinned);
			PawnCaptures(position, moves, pinned);
			PawnEnPassant(position, moves);
			PawnPromotions(position, moves, pinned);

			KingEvasions(position, moves);
			KingCapturesEvade(position, moves);
			CaptureThreat(position, moves, Threats);
			BlockThreat(position, moves, Threats);
		}
	}
	else
	{
		moves.reserve(50);
		GenerateLegalMoves(position, moves, pinned);
	}
}

void QuiescenceMoves(Position& position, std::vector<Move>& moves)
{
	moves.reserve(15);
	AddQuiescenceMoves(position, moves, PinnedMask(position));
}

void AddQuiescenceMoves(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	PawnCaptures(position, moves, pinned);
	PawnEnPassant(position, moves);
	PawnPromotions(position, moves, pinned);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, bitScanForwardErase(pieces), KnightAttacks, false, pinned));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, bitScanForwardErase(pieces), BishopAttacks, true, pinned));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, bitScanForwardErase(pieces), KingAttacks, false, pinned));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, bitScanForwardErase(pieces), RookAttacks, true, pinned));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, bitScanForwardErase(pieces), QueenAttacks, true, pinned));
}

uint64_t PinnedMask(const Position& position)
{
	bool turn = position.GetTurn();
	unsigned int king = position.GetKing(turn);
	if (IsSquareThreatened(position, king, turn)) return UNIVERCE;

	uint64_t mask = EMPTY;
	uint64_t possiblePins = QueenAttacks[king] & position.GetPiecesColour(turn);
	uint64_t maskAll = position.GetAllPieces();

	while (possiblePins != 0)
	{
		unsigned int sq = bitScanForwardErase(possiblePins);

		if (!mayMove(king, sq, maskAll))	//if you can't move from the square to the king, it can't be pinned
			continue;

		//If a piece is moving from the same diagonal as the king, and that diagonal contains an enemy bishop or queen
		if ((GetDiagonal(king) == GetDiagonal(sq)) && (DiagonalBB[GetDiagonal(king)] & (position.GetPieceBB(BISHOP, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			mask |= SquareBB[sq];
			continue;
		}

		//If a piece is moving from the same anti-diagonal as the king, and that diagonal contains an enemy bishop or queen
		if ((GetAntiDiagonal(king) == GetAntiDiagonal(sq)) && (AntiDiagonalBB[GetAntiDiagonal(king)] & (position.GetPieceBB(BISHOP, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			mask |= SquareBB[sq];
			continue;
		}

		//If a piece is moving from the same file as the king, and that file contains an enemy rook or queen
		if ((GetFile(king) == GetFile(sq)) && (FileBB[GetFile(king)] & (position.GetPieceBB(ROOK, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			mask |= SquareBB[sq];
			continue;
		}

		//If a piece is moving from the same rank as the king, and that rank contains an enemy rook or queen
		if ((GetRank(king) == GetRank(sq)) && (RankBB[GetRank(king)] & (position.GetPieceBB(ROOK, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			mask |= SquareBB[sq];
			continue;
		}
	}

	mask |= SquareBB[king];
	return mask;
}

void KingEvasions(Position& position, std::vector<Move>& moves)
{
	unsigned int square = position.GetKing(position.GetTurn());
	uint64_t quiet = position.GetEmptySquares() & KingAttacks[square];

	while (quiet != 0)
	{
		unsigned int target = bitScanForwardErase(quiet);
		Move move(square, target, QUIET);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void KingCapturesEvade(Position& position, std::vector<Move>& moves)
{
	unsigned int square = position.GetKing(position.GetTurn());
	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & KingAttacks[square];

	while (captures != 0)
	{
		unsigned int target = bitScanForwardErase(captures);
		Move move(square, target, CAPTURE);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void CaptureThreat(Position& position, std::vector<Move>& moves, uint64_t threats)
{
	unsigned int threatSquare = bitScanForwardErase(threats);

	uint64_t potentialCaptures = GetThreats(position, threatSquare, !position.GetTurn()) 
		& ~SquareBB[position.GetKing(position.GetTurn())]									//King captures handelled in KingCapturesEvade()
		& ~position.GetPieceBB(Piece(PAWN, position.GetTurn()));							//Pawn captures handelled elsewhere

	while (potentialCaptures != 0)
	{
		unsigned int start = bitScanForwardErase(potentialCaptures);
		Move move(start, threatSquare, CAPTURE);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void BlockThreat(Position& position, std::vector<Move>& moves, uint64_t threats)
{
	unsigned int threatSquare = bitScanForwardErase(threats);
	unsigned int piece = position.GetSquare(threatSquare);

	if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT) return;	//cant block non-sliders. Also cant be threatened by enemy king

	uint64_t blockSquares = inBetweenCache(threatSquare, position.GetKing(position.GetTurn()));

	while (blockSquares != 0)
	{
		unsigned int sq = bitScanForwardErase(blockSquares);
		uint64_t potentialBlockers = GetThreats(position, sq, !position.GetTurn()) & ~position.GetPieceBB(Piece(PAWN, position.GetTurn()));	//pawn moves need to be handelled elsewhere because they might threaten a square without being able to move there

		while (potentialBlockers != 0)
		{
			unsigned int start = bitScanForwardErase(potentialBlockers);
			Move move(start, sq, QUIET);

			if (!MovePutsSelfInCheck(position, move))
				moves.push_back(move);
		}
	}
}

void GenerateLegalMoves(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	PawnPushes(position, moves, pinned);
	PawnDoublePushes(position, moves, pinned);
	CastleMoves(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, bitScanForwardErase(pieces), KnightAttacks, false, pinned));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, bitScanForwardErase(pieces), BishopAttacks, true, pinned));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, bitScanForwardErase(pieces), QueenAttacks, true, pinned));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, bitScanForwardErase(pieces), RookAttacks, true, pinned));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, bitScanForwardErase(pieces), KingAttacks, false, pinned));

	AddQuiescenceMoves(position, moves, pinned);
}

void PawnPushes(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	int foward = 0;
	uint64_t targets = 0;
	uint64_t pawnSquares = position.GetPieceBB(PAWN, position.GetTurn());

	if (position.GetTurn() == WHITE)
	{
		foward = 8;
		targets = (pawnSquares << 8) & position.GetEmptySquares();
	}
	if (position.GetTurn() == BLACK)
	{
		foward = -8;
		targets = (pawnSquares >> 8) & position.GetEmptySquares();
	}

	uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]);			//pushes that aren't to the back ranks

	while (pawnPushes != 0)
	{
		unsigned int end = bitScanForwardErase(pawnPushes);
		Move move(end - foward, end, QUIET);

		if (!(pinned & SquareBB[end- foward]) || !MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void PawnPromotions(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	int foward = 0;
	uint64_t targets = 0;
	uint64_t pawnSquares = position.GetPieceBB(PAWN, position.GetTurn());

	if (position.GetTurn() == WHITE)
	{
		foward = 8;
		targets = (pawnSquares << 8) & position.GetEmptySquares();
	}
	if (position.GetTurn() == BLACK)
	{
		foward = -8;
		targets = (pawnSquares >> 8) & position.GetEmptySquares();
	}

	uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]);			//pushes that are to the back ranks

	while (pawnPromotions != 0)
	{
		unsigned int end = bitScanForwardErase(pawnPromotions);

		Move move(end - foward, end, KNIGHT_PROMOTION);
		if ((pinned & SquareBB[end - foward]) && MovePutsSelfInCheck(position, move))
			continue;

		moves.push_back(move);
		moves.push_back(Move(end - foward, end, ROOK_PROMOTION));
		moves.push_back(Move(end - foward, end, BISHOP_PROMOTION));
		moves.push_back(Move(end - foward, end, QUEEN_PROMOTION));
	}
}

void PawnDoublePushes(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	int foward = 0;
	uint64_t targets = 0;
	uint64_t pawnSquares = position.GetPieceBB(PAWN, position.GetTurn());

	if (position.GetTurn() == WHITE)
	{
		foward = 16;
		pawnSquares &= RankBB[RANK_2];
		targets = (pawnSquares << 8) & position.GetEmptySquares();
		targets = (targets << 8) & position.GetEmptySquares();
	}
	if (position.GetTurn() == BLACK)
	{
		foward = -16;
		pawnSquares &= RankBB[RANK_7];
		targets = (pawnSquares >> 8) & position.GetEmptySquares();
		targets = (targets >> 8) & position.GetEmptySquares();
	}

	while (targets != 0)
	{
		unsigned int end = bitScanForwardErase(targets);
		Move move(end - foward, end, PAWN_DOUBLE_MOVE);

		if (!(pinned & SquareBB[end - foward]) || !MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void PawnEnPassant(Position& position, std::vector<Move>& moves)
{
	if (position.GetEnPassant() <= SQ_H8)
	{
		if (position.GetTurn() == WHITE)
		{
			uint64_t potentialAttackers = BlackPawnAttacks[position.GetEnPassant()] & position.GetPieceBB(WHITE_PAWN);			//if a black pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanForwardErase(potentialAttackers);

				Move move(start, position.GetEnPassant(), EN_PASSANT);
				if (!MovePutsSelfInCheck(position, move))
					moves.push_back(move);
			}
		}

		if (position.GetTurn() == BLACK)
		{
			uint64_t potentialAttackers = WhitePawnAttacks[position.GetEnPassant()] & position.GetPieceBB(BLACK_PAWN);			//if a white pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanForwardErase(potentialAttackers);

				Move move(start, position.GetEnPassant(), EN_PASSANT);
				if (!MovePutsSelfInCheck(position, move))
					moves.push_back(move);
			}
		}
	}
}

void PawnCaptures(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	int fowardleft = 0;
	int fowardright = 0;
	uint64_t leftAttack = 0;
	uint64_t rightAttack = 0;
	uint64_t pawnSquares = position.GetPieceBB(PAWN, position.GetTurn());

	if (position.GetTurn() == WHITE)
	{
		fowardleft = 7;
		fowardright = 9;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) << 7) & position.GetBlackPieces();
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) << 9) & position.GetBlackPieces();
	}
	if (position.GetTurn() == BLACK)
	{
		fowardleft = -9;
		fowardright = -7;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) >> 9) & position.GetWhitePieces();
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) >> 7) & position.GetWhitePieces();
	}

	while (leftAttack != 0)
	{
		unsigned int end = bitScanForwardErase(leftAttack);

		Move move(end - fowardleft, end, CAPTURE);
		if ((pinned & SquareBB[end - fowardleft]) && MovePutsSelfInCheck(position, move))
			continue;

		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardleft, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(move);
	}

	while (rightAttack != 0)
	{
		unsigned int end = bitScanForwardErase(rightAttack);

		Move move(end - fowardright, end, CAPTURE);
		if ((pinned & SquareBB[end - fowardright]) && MovePutsSelfInCheck(position, move))
			continue;

		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardright, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(move);
	}
}

void CastleMoves(const Position& position, std::vector<Move>& moves)
{
	uint64_t Pieces = position.GetAllPieces();

	if (position.CanCastleWhiteKingside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_H1, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E1, position.GetTurn()) && !IsSquareThreatened(position, SQ_F1, position.GetTurn()) && !IsSquareThreatened(position, SQ_G1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_G1, KING_CASTLE));
			}
		}
	}

	if (position.CanCastleWhiteQueenside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_A1, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E1, position.GetTurn()) && !IsSquareThreatened(position, SQ_D1, position.GetTurn()) && !IsSquareThreatened(position, SQ_C1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_C1, QUEEN_CASTLE));
			}
		}
	}

	if (position.CanCastleBlackKingside() && position.GetTurn() == BLACK)
	{
		if (mayMove(SQ_E8, SQ_H8, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E8, position.GetTurn()) && !IsSquareThreatened(position, SQ_F8, position.GetTurn()) && !IsSquareThreatened(position, SQ_G8, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E8, SQ_G8, KING_CASTLE));
			}
		}
	}

	if (position.CanCastleBlackQueenside() && position.GetTurn() == BLACK)
	{
		if (mayMove(SQ_E8, SQ_A8, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E8, position.GetTurn()) && !IsSquareThreatened(position, SQ_D8, position.GetTurn()) && !IsSquareThreatened(position, SQ_C8, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E8, SQ_C8, QUEEN_CASTLE));
			}
		}
	}
}

void GenerateQuietMoves(Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding, uint64_t pinned)
{
	assert(square < N_SQUARES);

	uint64_t quiet = position.GetEmptySquares() & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (quiet != 0)
	{
		unsigned int target = bitScanForwardErase(quiet);
		if (!isSliding || mayMove(square, target, maskall))
		{
			Move move(square, target, QUIET);

			if ((pinned & SquareBB[square]) && MovePutsSelfInCheck(position, move))
				continue;

			moves.push_back(move);
		}
	}
}

void GenerateCaptureMoves(Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding, uint64_t pinned)
{
	assert(square < N_SQUARES);

	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (captures != 0)
	{
		unsigned int target = bitScanForwardErase(captures);
		if (!isSliding || mayMove(square, target, maskall))
		{
			Move move(square, target, CAPTURE);

			if ((pinned & SquareBB[square]) && MovePutsSelfInCheck(position, move))
				continue;

			moves.push_back(move);
		}
	}
}

bool IsSquareThreatened(const Position& position, unsigned int square, bool colour)
{
	assert(square < N_SQUARES);

	if ((KnightAttacks[square] & position.GetPieceBB(KNIGHT, !colour)) != 0)
		return true;

	if (colour == WHITE)
	{
		if ((WhitePawnAttacks[square] & position.GetPieceBB(BLACK_PAWN)) != 0)
			return true;
	}

	if (colour == BLACK)
	{
		if ((BlackPawnAttacks[square] & position.GetPieceBB(WHITE_PAWN)) != 0)
			return true;
	}

	if ((KingAttacks[square] & position.GetPieceBB(KING, !colour)) != 0)					//if I can attack the enemy king he can attack me
		return true;

	uint64_t Pieces = position.GetAllPieces();
	
	uint64_t queen = position.GetPieceBB(QUEEN, !colour) & QueenAttacks[square];
	while (queen != 0)
	{
		unsigned int start = bitScanForwardErase(queen);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t bishops = position.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = bitScanForwardErase(bishops);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t rook = position.GetPieceBB(ROOK, !colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = bitScanForwardErase(rook);
		if (mayMove(start, square, Pieces))
			return true;
	}

	return false;
}

bool IsInCheck(const Position& position, bool colour)
{
	return IsSquareThreatened(position, position.GetKing(colour), colour);
}

bool IsInCheck(const Position& position)
{
	return IsInCheck(position, position.GetTurn());
}

uint64_t GetThreats(const Position& position, unsigned int square, bool colour)
{
	assert(square < N_SQUARES);
	uint64_t threats = EMPTY;

	threats |= (KnightAttacks[square] & position.GetPieceBB(KNIGHT, !colour));

	if (colour == WHITE)
	{
		threats |= (WhitePawnAttacks[square] & position.GetPieceBB(BLACK_PAWN));
	}

	if (colour == BLACK)
	{
		threats |= (BlackPawnAttacks[square] & position.GetPieceBB(WHITE_PAWN));
	}

	threats |= (KingAttacks[square] & position.GetPieceBB(KING, !colour));					//if I can attack the enemy king he can attack me

	uint64_t Pieces = position.GetAllPieces();

	uint64_t queen = position.GetPieceBB(QUEEN, !colour) & QueenAttacks[square];
	while (queen != 0)
	{
		unsigned int start = bitScanForwardErase(queen);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	uint64_t bishops = position.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = bitScanForwardErase(bishops);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	uint64_t rook = position.GetPieceBB(ROOK, !colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = bitScanForwardErase(rook);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	return threats;
}

Move GetSmallestAttackerMove(const Position& position, unsigned int square, bool colour)
{
	assert(square < N_SQUARES);

	uint64_t pawnmask = 0;
	if (colour == BLACK)
	{
		pawnmask = (WhitePawnAttacks[square] & position.GetPieceBB(BLACK_PAWN));
	}

	if (colour == WHITE)
	{
		pawnmask = (BlackPawnAttacks[square] & position.GetPieceBB(WHITE_PAWN));
	}

	if (pawnmask != 0)
	{
		return(Move(bitScanForwardErase(pawnmask), square, CAPTURE));
	}

	uint64_t knightmask = (KnightAttacks[square] & position.GetPieceBB(KNIGHT, colour));
	if (knightmask != 0)
	{
		return(Move(bitScanForwardErase(knightmask), square, CAPTURE));
	}

	uint64_t Pieces = position.GetAllPieces();

	uint64_t bishops = position.GetPieceBB(BISHOP, colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = bitScanForwardErase(bishops);
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t rook = position.GetPieceBB(ROOK, colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = bitScanForwardErase(rook);
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t queen = position.GetPieceBB(QUEEN, colour) & QueenAttacks[square];
	while (queen != 0)
	{
		unsigned int start = bitScanForwardErase(queen);
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t kingmask = (KingAttacks[square] & position.GetPieceBB(KING, colour));
	if (kingmask != 0)
	{
		return(Move(bitScanForwardErase(kingmask), square, CAPTURE));
	}

	return Move();
}

bool MoveIsLegal(Position& position, Move& move)
{
	/*Obvious check first*/
	if (move.IsUninitialized())
		return false;

	unsigned int Piece = position.GetSquare(move.GetFrom());

	/*Make sure there's a piece to be moved*/
	if (position.GetSquare(move.GetFrom()) == N_PIECES)
		return false;

	/*Make sure the piece are are moving is ours*/
	if (ColourOfPiece(Piece) != position.GetTurn())
		return false;

	/*Make sure we aren't capturing our own piece*/
	if (position.GetSquare(move.GetTo()) != N_PIECES && ColourOfPiece(position.GetSquare(move.GetTo())) == position.GetTurn())
		return false;

	/*If capture, make sure there's something there to capture*/
	if (move.IsCapture() && move.GetFlag() != EN_PASSANT)
		if (position.GetSquare(move.GetTo()) == N_PIECES)
			return false;

	/*If promotion, make sure there's a pawn we are moving*/
	if (move.IsPromotion())
		if (position.GetSquare(move.GetFrom()) != WHITE_PAWN && position.GetSquare(move.GetFrom()) != BLACK_PAWN)
			return false;

	uint64_t allPieces = position.GetAllPieces();

	/*Sliding pieces*/
	if (Piece == WHITE_BISHOP || Piece == BLACK_BISHOP || Piece == WHITE_ROOK || Piece == BLACK_ROOK || Piece == WHITE_QUEEN || Piece == BLACK_QUEEN)
	{
		if (!mayMove(move.GetFrom(), move.GetTo(), allPieces))
			return false;
	}

	if (Piece == WHITE_PAWN)
	{
		if (RankDiff(move.GetFrom(), move.GetTo()) == -1 && FileDiff(move.GetFrom(), move.GetTo()) == 0)		//push
		{
			if (position.GetSquare(move.GetTo()) != N_PIECES)		//Something in the way!
				return false;
		}
		else if (RankDiff(move.GetFrom(), move.GetTo()) == -2 && FileDiff(move.GetFrom(), move.GetTo()) == 0)	//double push
		{
			if (GetRank(move.GetFrom()) != RANK_2)
				return false;

			if (position.GetSquare(move.GetTo()) != N_PIECES)								//Something in the way!
				return false;

			if (position.GetSquare((move.GetTo() + move.GetFrom()) / 2) != N_PIECES)		//average of from and to is the middle square
				return false;
		}
		else if (RankDiff(move.GetFrom(), move.GetTo()) == -1 && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1)	//capture
		{
			if (position.GetSquare(move.GetTo()) == N_PIECES && position.GetEnPassant() != move.GetTo())		//nothing there to capture
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (Piece == BLACK_PAWN)
	{
		if (RankDiff(move.GetFrom(), move.GetTo()) == 1 && FileDiff(move.GetFrom(), move.GetTo()) == 0)		//push
		{
			if (position.GetSquare(move.GetTo()) != N_PIECES)		//Something in the way!
				return false;
		}
		else if (RankDiff(move.GetFrom(), move.GetTo()) == 2 && FileDiff(move.GetFrom(), move.GetTo()) == 0)	//double push
		{
			if (GetRank(move.GetFrom()) != RANK_7)
				return false;

			if (position.GetSquare(move.GetTo()) != N_PIECES)								//Something in the way!
				return false;

			if (position.GetSquare((move.GetTo() + move.GetFrom()) / 2) != N_PIECES)		//average of from and to is the middle square
				return false;
		}
		else if (RankDiff(move.GetFrom(), move.GetTo()) == 1 && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1)	//capture
		{
			if (position.GetSquare(move.GetTo()) == N_PIECES && position.GetEnPassant() != move.GetTo())		//nothing there to capture
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (Piece == WHITE_KNIGHT || Piece == BLACK_KNIGHT)
	{
		if ((SquareBB[move.GetTo()] & KnightAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if ((Piece == WHITE_KING || Piece == BLACK_KING) && !(move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE))
	{
		if ((SquareBB[move.GetTo()] & KingAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (Piece == WHITE_ROOK || Piece == BLACK_ROOK)
	{
		if ((SquareBB[move.GetTo()] & RookAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (Piece == WHITE_BISHOP || Piece == BLACK_BISHOP)
	{
		if ((SquareBB[move.GetTo()] & BishopAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (Piece == WHITE_QUEEN || Piece == BLACK_QUEEN)
	{
		if ((SquareBB[move.GetTo()] & QueenAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE)
	{
		std::vector<Move> moves;
		CastleMoves(position, moves);

		bool present = false;
		for (int i = 0; i < moves.size(); i++)
		{
			if (moves[i] == move)
				present = true;
		}

		if (!present)
			return false;
	}

	/*Move puts me in check*/
	if (MovePutsSelfInCheck(position, move))
		return false;

	return true;
}

bool MovePutsSelfInCheck(Position& position, const Move & move)
{
	unsigned int fromPiece = position.GetSquare(move.GetFrom());
	unsigned int toPiece = position.GetSquare(move.GetTo());
	unsigned int epPiece = static_cast<unsigned int>(-1);

	position.SetSquare(move.GetTo(), fromPiece);
	position.ClearSquare(move.GetFrom());

	if (move.GetFlag() == EN_PASSANT)
	{
		epPiece = position.GetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		position.ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
	}

	bool InCheck = IsSquareThreatened(position, position.GetKing(position.GetTurn()), position.GetTurn());							//CANNOT use 'king' in place of GetKing because the king may have moved.

	if (move.GetFlag() == EN_PASSANT)
	{
		position.ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		position.SetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())), epPiece);
	}

	position.SetSquare(move.GetFrom(), fromPiece);

	if (toPiece != N_PIECES)
		position.SetSquare(move.GetTo(), toPiece);
	else
		position.ClearSquare(move.GetTo());

	return InCheck;
}
