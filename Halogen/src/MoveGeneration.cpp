#include "MoveGeneration.h"

void GenerateLegalMoves(Position& position, std::vector<Move>& moves, uint64_t pinned);
void AddQuiescenceMoves(Position& position, std::vector<Move>& moves, uint64_t pinned);	//captures and/or promotions
void AddQuietMoves(Position& position, std::vector<Move>& moves, uint64_t pinned);

//Pawn moves
void PawnPushes(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnPromotions(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnDoublePushes(Position& position, std::vector<Move>& moves, uint64_t pinned);
void PawnEnPassant(Position& position, std::vector<Move>& moves);	//Ep moves are always checked for legality so no need for pinned mask
void PawnCaptures(Position& position, std::vector<Move>& moves, uint64_t pinned);

//All other pieces
void GenerateQuietMoves(Position& position, std::vector<Move>& moves, Square square, PieceTypes pieceType, uint64_t pinned);
void GenerateCaptureMoves(Position& position, std::vector<Move>& moves, Square square, PieceTypes pieceType, uint64_t pinned);

//misc
void CastleMoves(const Position& position, std::vector<Move>& moves);

//utility functions
bool MovePutsSelfInCheck(Position& position, const Move& move);
uint64_t PinnedMask(const Position& position);
uint64_t AttackBB(PieceTypes piecetype, Square sq, uint64_t occupied);

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

void QuietMoves(Position& position, std::vector<Move>& moves)
{
	moves.reserve(40);
	AddQuietMoves(position, moves, PinnedMask(position));
}

void AddQuiescenceMoves(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	PawnCaptures(position, moves, pinned);
	PawnEnPassant(position, moves);
	PawnPromotions(position, moves, pinned);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, static_cast<Square>(LSPpop(pieces)), KNIGHT, pinned));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; GenerateCaptureMoves(position, moves, static_cast<Square>(LSPpop(pieces)), BISHOP, pinned));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn());   pieces != 0; GenerateCaptureMoves(position, moves, static_cast<Square>(LSPpop(pieces)), KING, pinned));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn());   pieces != 0; GenerateCaptureMoves(position, moves, static_cast<Square>(LSPpop(pieces)), ROOK, pinned));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn());  pieces != 0; GenerateCaptureMoves(position, moves, static_cast<Square>(LSPpop(pieces)), QUEEN, pinned));
}

uint64_t PinnedMask(const Position& position)
{
	Players turn = position.GetTurn();
	Square king = position.GetKing(turn);
	if (IsSquareThreatened(position, king, turn)) return UNIVERCE;

	uint64_t mask = EMPTY;
	uint64_t possiblePins = QueenAttacks[king] & position.GetPiecesColour(turn);
	uint64_t maskAll = position.GetAllPieces();

	while (possiblePins != 0)
	{
		unsigned int sq = LSPpop(possiblePins);

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
	Square square = position.GetKing(position.GetTurn());
	uint64_t quiet = position.GetEmptySquares() & KingAttacks[square];

	while (quiet != 0)
	{
		Square target = static_cast<Square>(LSPpop(quiet));
		Move move(square, target, QUIET);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void KingCapturesEvade(Position& position, std::vector<Move>& moves)
{
	Square square = position.GetKing(position.GetTurn());
	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & KingAttacks[square];

	while (captures != 0)
	{
		Square target = static_cast<Square>(LSPpop(captures));
		Move move(square, target, CAPTURE);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void CaptureThreat(Position& position, std::vector<Move>& moves, uint64_t threats)
{
	Square threatSquare = static_cast<Square>(LSPpop(threats));

	uint64_t potentialCaptures = GetThreats(position, threatSquare, !position.GetTurn()) 
		& ~SquareBB[position.GetKing(position.GetTurn())]									//King captures handelled in KingCapturesEvade()
		& ~position.GetPieceBB(Piece(PAWN, position.GetTurn()));							//Pawn captures handelled elsewhere

	while (potentialCaptures != 0)
	{
		Square start = static_cast<Square>(LSPpop(potentialCaptures));
		Move move(start, threatSquare, CAPTURE);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}
}

void BlockThreat(Position& position, std::vector<Move>& moves, uint64_t threats)
{
	Square threatSquare = static_cast<Square>(LSPpop(threats));
	Pieces piece = position.GetSquare(threatSquare);

	if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT) return;	//cant block non-sliders. Also cant be threatened by enemy king

	uint64_t blockSquares = inBetweenCache(threatSquare, position.GetKing(position.GetTurn()));

	while (blockSquares != 0)
	{
		Square sq = static_cast<Square>(LSPpop(blockSquares));
		uint64_t potentialBlockers = GetThreats(position, sq, !position.GetTurn()) & ~position.GetPieceBB(Piece(PAWN, position.GetTurn()));	//pawn moves need to be handelled elsewhere because they might threaten a square without being able to move there

		while (potentialBlockers != 0)
		{
			Square start = static_cast<Square>(LSPpop(potentialBlockers));
			Move move(start, sq, QUIET);

			if (!MovePutsSelfInCheck(position, move))
				moves.push_back(move);
		}
	}
}

void GenerateLegalMoves(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	AddQuietMoves(position, moves, pinned);
	AddQuiescenceMoves(position, moves, pinned);
}

void AddQuietMoves(Position& position, std::vector<Move>& moves, uint64_t pinned)
{
	PawnPushes(position, moves, pinned);
	PawnDoublePushes(position, moves, pinned);
	CastleMoves(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, static_cast<Square>(LSPpop(pieces)), KNIGHT, pinned));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, static_cast<Square>(LSPpop(pieces)), BISHOP, pinned));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, static_cast<Square>(LSPpop(pieces)), QUEEN, pinned));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, static_cast<Square>(LSPpop(pieces)), ROOK, pinned));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; GenerateQuietMoves(position, moves, static_cast<Square>(LSPpop(pieces)), KING, pinned));
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
		Square end = static_cast<Square>(LSPpop(pawnPushes));
		Move move(static_cast<Square>(end - foward), end, QUIET);

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
		Square end = static_cast<Square>(LSPpop(pawnPromotions));

		Move move(static_cast<Square>(end - foward), end, KNIGHT_PROMOTION);
		if ((pinned & SquareBB[end - foward]) && MovePutsSelfInCheck(position, move))
			continue;

		moves.push_back(move);
		moves.push_back(Move(static_cast<Square>(end - foward), end, ROOK_PROMOTION));
		moves.push_back(Move(static_cast<Square>(end - foward), end, BISHOP_PROMOTION));
		moves.push_back(Move(static_cast<Square>(end - foward), end, QUEEN_PROMOTION));
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
		Square end = static_cast<Square>(LSPpop(targets));
		Move move(static_cast<Square>(end - foward), end, PAWN_DOUBLE_MOVE);

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
				Square start = static_cast<Square>(LSPpop(potentialAttackers));

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
				Square start = static_cast<Square>(LSPpop(potentialAttackers));

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
		Square end = static_cast<Square>(LSPpop(leftAttack));

		Move move(static_cast<Square>(end - fowardleft), end, CAPTURE);
		if ((pinned & SquareBB[end - fowardleft]) && MovePutsSelfInCheck(position, move))
			continue;

		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(static_cast<Square>(end - fowardleft), end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardleft), end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardleft), end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardleft), end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(move);
	}

	while (rightAttack != 0)
	{
		Square end = static_cast<Square>(LSPpop(rightAttack));

		Move move(static_cast<Square>(end - fowardright), end, CAPTURE);
		if ((pinned & SquareBB[end - fowardright]) && MovePutsSelfInCheck(position, move))
			continue;

		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(static_cast<Square>(end - fowardright), end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardright), end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardright), end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(static_cast<Square>(end - fowardright), end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(move);
	}
}

void CastleMoves(const Position& position, std::vector<Move>& moves)
{
	uint64_t Pieces = position.GetAllPieces();

	if (position.GetCanCastleWhiteKingside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_H1, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E1, position.GetTurn()) && !IsSquareThreatened(position, SQ_F1, position.GetTurn()) && !IsSquareThreatened(position, SQ_G1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_G1, KING_CASTLE));
			}
		}
	}

	if (position.GetCanCastleWhiteQueenside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_A1, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E1, position.GetTurn()) && !IsSquareThreatened(position, SQ_D1, position.GetTurn()) && !IsSquareThreatened(position, SQ_C1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_C1, QUEEN_CASTLE));
			}
		}
	}

	if (position.GetCanCastleBlackKingside() && position.GetTurn() == BLACK)
	{
		if (mayMove(SQ_E8, SQ_H8, Pieces))
		{
			if (!IsSquareThreatened(position, SQ_E8, position.GetTurn()) && !IsSquareThreatened(position, SQ_F8, position.GetTurn()) && !IsSquareThreatened(position, SQ_G8, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E8, SQ_G8, KING_CASTLE));
			}
		}
	}

	if (position.GetCanCastleBlackQueenside() && position.GetTurn() == BLACK)
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

void GenerateQuietMoves(Position& position, std::vector<Move>& moves, Square square, PieceTypes pieceType, uint64_t pinned)
{
	assert(square < N_SQUARES);

	uint64_t occupied = position.GetAllPieces();
	uint64_t targets = ~occupied;
	uint64_t quiet = targets & AttackBB(pieceType, square, occupied);

	while (quiet != 0)
	{
		Square target = static_cast<Square>(LSPpop(quiet));
		Move move(square, target, QUIET);

		if ((pinned & SquareBB[square]) && MovePutsSelfInCheck(position, move))
			continue;

		moves.push_back(move);
	}
}

void GenerateCaptureMoves(Position& position, std::vector<Move>& moves, Square square, PieceTypes pieceType, uint64_t pinned)
{
	assert(square < N_SQUARES);

	uint64_t occupied = position.GetAllPieces();
	uint64_t targets = position.GetPiecesColour(!position.GetTurn());
	uint64_t quiet = targets & AttackBB(pieceType, square, occupied);

	while (quiet != 0)
	{
		Square target = static_cast<Square>(LSPpop(quiet));
		Move move(square, target, CAPTURE);

		if ((pinned & SquareBB[square]) && MovePutsSelfInCheck(position, move))
			continue;

		moves.push_back(move);
	}
}

bool IsSquareThreatened(const Position& position, Square square, Players colour)
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
		unsigned int start = LSPpop(queen);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t bishops = position.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = LSPpop(bishops);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t rook = position.GetPieceBB(ROOK, !colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = LSPpop(rook);
		if (mayMove(start, square, Pieces))
			return true;
	}

	return false;
}

bool IsInCheck(const Position& position, Players colour)
{
	return IsSquareThreatened(position, position.GetKing(colour), colour);
}

bool IsInCheck(const Position& position)
{
	return IsInCheck(position, position.GetTurn());
}

uint64_t GetThreats(const Position& position, Square square, Players colour)
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
		unsigned int start = LSPpop(queen);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	uint64_t bishops = position.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = LSPpop(bishops);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	uint64_t rook = position.GetPieceBB(ROOK, !colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = LSPpop(rook);
		if (mayMove(start, square, Pieces))
			threats |= SquareBB[start];
	}

	return threats;
}

Move GetSmallestAttackerMove(const Position& position, Square square, Players colour)
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
		return(Move(static_cast<Square>(LSPpop(pawnmask)), square, CAPTURE));
	}

	uint64_t knightmask = (KnightAttacks[square] & position.GetPieceBB(KNIGHT, colour));
	if (knightmask != 0)
	{
		return(Move(static_cast<Square>(LSPpop(knightmask)), square, CAPTURE));
	}

	uint64_t Pieces = position.GetAllPieces();

	uint64_t bishops = position.GetPieceBB(BISHOP, colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		Square start = static_cast<Square>(LSPpop(bishops));
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t rook = position.GetPieceBB(ROOK, colour) & RookAttacks[square];
	while (rook != 0)
	{
		Square start = static_cast<Square>(LSPpop(rook));
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t queen = position.GetPieceBB(QUEEN, colour) & QueenAttacks[square];
	while (queen != 0)
	{
		Square start = static_cast<Square>(LSPpop(queen));
		if (mayMove(start, square, Pieces))
			return Move(start, square, CAPTURE);
	}

	uint64_t kingmask = (KingAttacks[square] & position.GetPieceBB(KING, colour));
	if (kingmask != 0)
	{
		return(Move(static_cast<Square>(LSPpop(kingmask)), square, CAPTURE));
	}

	return Move();
}

bool MoveIsLegal(Position& position, const Move& move)
{
	/*Obvious check first*/
	if (move.IsUninitialized())
		return false;

	unsigned int piece = position.GetSquare(move.GetFrom());

	/*Make sure there's a piece to be moved*/
	if (position.GetSquare(move.GetFrom()) == N_PIECES)
		return false;

	/*Make sure the piece are are moving is ours*/
	if (ColourOfPiece(piece) != position.GetTurn())
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
	if (piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_ROOK || piece == BLACK_ROOK || piece == WHITE_QUEEN || piece == BLACK_QUEEN)
	{
		if (!mayMove(move.GetFrom(), move.GetTo(), allPieces))
			return false;
	}

	if (piece == WHITE_PAWN)
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

			if (position.GetSquare(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2)) != N_PIECES)		//average of from and to is the middle square
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

	if (piece == BLACK_PAWN)
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

			if (position.GetSquare(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2)) != N_PIECES)		//average of from and to is the middle square
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

	if (piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
	{
		if ((SquareBB[move.GetTo()] & KnightAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if ((piece == WHITE_KING || piece == BLACK_KING) && !(move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE))
	{
		if ((SquareBB[move.GetTo()] & KingAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (piece == WHITE_ROOK || piece == BLACK_ROOK)
	{
		if ((SquareBB[move.GetTo()] & RookAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (piece == WHITE_BISHOP || piece == BLACK_BISHOP)
	{
		if ((SquareBB[move.GetTo()] & BishopAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (piece == WHITE_QUEEN || piece == BLACK_QUEEN)
	{
		if ((SquareBB[move.GetTo()] & QueenAttacks[move.GetFrom()]) == 0)
			return false;
	}

	if (move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE)
	{
		std::vector<Move> moves;
		CastleMoves(position, moves);

		for (size_t i = 0; i < moves.size(); i++)
		{
			if (moves[i] == move)
				return true;
		}
		return false;
	}

	/*Move puts me in check*/
	if (MovePutsSelfInCheck(position, move))
		return false;

	return true;
}

/*
This function does not work for casteling moves. They are tested for legality their creation.
*/
bool MovePutsSelfInCheck(Position& position, const Move & move)
{
	assert(move.GetFlag() != KING_CASTLE);
	assert(move.GetFlag() != QUEEN_CASTLE);

	position.ApplyMoveQuick(move);
	bool ret = IsInCheck(position, position.GetTurn());
	position.RevertMoveQuick();
	return ret;
}

//--------------------------------------------------------------------------
//Below code adapted with permission from Terje, author of Weiss.

// Returns the attack bitboard for a piece of piecetype on square sq
uint64_t AttackBB(PieceTypes piecetype, Square sq, uint64_t occupied) 
{
	switch (piecetype) {
	case KNIGHT:	return KnightAttacks[sq];
	case KING:		return KingAttacks[sq];
	case BISHOP:	return BishopTable[sq].attacks[AttackIndex(sq, occupied, BishopTable)];
	case ROOK:		return   RookTable[sq].attacks[AttackIndex(sq, occupied, RookTable)];
	case QUEEN:		return AttackBB(ROOK, sq, occupied) | AttackBB(BISHOP, sq, occupied);
	}

	throw std::invalid_argument("piecetype is argument is invalid");
	return 0;
}

//--------------------------------------------------------------------------