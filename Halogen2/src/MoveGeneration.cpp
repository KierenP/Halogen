#include "MoveGeneration.h"

void PawnPushes(const Position & position, std::vector<Move>& moves);
void PawnPromotions(const Position& position, std::vector<Move>& moves);
void PawnDoublePushes(const Position & position, std::vector<Move>& moves);
void PawnEnPassant(const Position & position, std::vector<Move>& moves);
void PawnCaptures(const Position & position, std::vector<Move>& moves);
void CastleMoves(const Position & position, std::vector<Move>& moves);

void CalculateQuietMovesBB(const Position & position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding);
void CalculateMovesBBCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding);

void RemoveIllegal(Position & position, std::vector<Move>& moves);	//remove all the moves that put the current player's king in check
bool MovePutsSelfInCheck(Position & position, Move & move);

void AddQuiescenceMoves(const Position& position, std::vector<Move>& moves);

//special generators for when in check
void KingEvasions(Position & position, std::vector<Move>& moves);
void CaptureThreat(Position& position, std::vector<Move>& moves, uint64_t threats);
void BlockThreat(Position& position, std::vector<Move>& moves, uint64_t threats);

void LegalMoves(Position & position, std::vector<Move>& moves)
{
	if (IsInCheck(position, position.GetTurn()))
	{
		moves.reserve(10);

		uint64_t Threats = GetThreats(position, position.GetKing(position.GetTurn()), position.GetTurn());

		assert(Threats != 0);

		if (GetBitCount(Threats) > 1)	//double check
		{
			KingEvasions(position, moves);
		}
		else
		{
			PawnPushes(position, moves);		//pawn moves are hard :( so we calculate those normally
			PawnDoublePushes(position, moves);
			PawnCaptures(position, moves);
			PawnEnPassant(position, moves);
			PawnPromotions(position, moves);

			RemoveIllegal(position, moves);

			KingEvasions(position, moves);
			CaptureThreat(position, moves, Threats);
			BlockThreat(position, moves, Threats);
		}
	}
	else
	{
		moves.reserve(50);

		GeneratePsudoLegalMoves(position, moves);
		RemoveIllegal(position, moves);
	}
}

void QuiescenceMoves(Position& position, std::vector<Move>& moves)
{
	moves.reserve(10);

	AddQuiescenceMoves(position, moves);
	RemoveIllegal(position, moves);
}

void AddQuiescenceMoves(const Position& position, std::vector<Move>& moves)
{
	PawnCaptures(position, moves);
	PawnEnPassant(position, moves);
	PawnPromotions(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanForwardErase(pieces), KnightAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanForwardErase(pieces), BishopAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanForwardErase(pieces), KingAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanForwardErase(pieces), RookAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanForwardErase(pieces), QueenAttacks, true));
}

void KingEvasions(Position & position, std::vector<Move>& moves)
{
	unsigned int square = position.GetKing(position.GetTurn());

	uint64_t quiet = position.GetEmptySquares() & KingAttacks[square];
	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & KingAttacks[square];

	while (quiet != 0)
	{
		unsigned int target = bitScanForwardErase(quiet);
		Move move(square, target, QUIET);

		if (!MovePutsSelfInCheck(position, move))
			moves.push_back(move);
	}

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
		& ~SquareBB[position.GetKing(position.GetTurn())]									//King captures handelled in KingEvasions()
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

void GeneratePsudoLegalMoves(const Position & position, std::vector<Move>& moves)
{
	PawnPushes(position, moves);
	PawnDoublePushes(position, moves);
	CastleMoves(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; CalculateQuietMovesBB(position, moves, bitScanForwardErase(pieces), KnightAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; CalculateQuietMovesBB(position, moves, bitScanForwardErase(pieces), KingAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; CalculateQuietMovesBB(position, moves, bitScanForwardErase(pieces), BishopAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; CalculateQuietMovesBB(position, moves, bitScanForwardErase(pieces), QueenAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; CalculateQuietMovesBB(position, moves, bitScanForwardErase(pieces), RookAttacks, true));

	AddQuiescenceMoves(position, moves);
}

void PawnPushes(const Position & position, std::vector<Move>& moves)
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
		moves.push_back(Move(end - foward, end, QUIET));
	}
}

void PawnPromotions(const Position& position, std::vector<Move>& moves)
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
		targets = (pawnSquares >> 8)& position.GetEmptySquares();
	}

	uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]);			//pushes that are to the back ranks

	while (pawnPromotions != 0)
	{
		unsigned int end = bitScanForwardErase(pawnPromotions);
		moves.push_back(Move(end - foward, end, KNIGHT_PROMOTION));
		moves.push_back(Move(end - foward, end, ROOK_PROMOTION));
		moves.push_back(Move(end - foward, end, BISHOP_PROMOTION));
		moves.push_back(Move(end - foward, end, QUEEN_PROMOTION));
	}
}

void PawnDoublePushes(const Position & position, std::vector<Move>& moves)
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
		moves.push_back(Move(end - foward, end, PAWN_DOUBLE_MOVE));
	}
}

void PawnEnPassant(const Position & position, std::vector<Move>& moves)
{
	if (position.GetEnPassant() <= SQ_H8)
	{
		if (position.GetTurn() == WHITE)
		{
			uint64_t potentialAttackers = BlackPawnAttacks[position.GetEnPassant()] & position.GetPieceBB(WHITE_PAWN);			//if a black pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanForwardErase(potentialAttackers);
				moves.push_back(Move(start, position.GetEnPassant(), EN_PASSANT));
			}
		}

		if (position.GetTurn() == BLACK)
		{
			uint64_t potentialAttackers = WhitePawnAttacks[position.GetEnPassant()] & position.GetPieceBB(BLACK_PAWN);			//if a white pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanForwardErase(potentialAttackers);
				moves.push_back(Move(start, position.GetEnPassant(), EN_PASSANT));
			}
		}
	}
}

void PawnCaptures(const Position & position, std::vector<Move>& moves)
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
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardleft, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(Move(end - fowardleft, end, CAPTURE));
	}

	while (rightAttack != 0)
	{
		unsigned int end = bitScanForwardErase(rightAttack);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardright, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, BISHOP_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, QUEEN_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(Move(end - fowardright, end, CAPTURE));
	}
}

void CastleMoves(const Position & position, std::vector<Move>& moves)
{
	uint64_t Pieces = position.GetAllPieces();

	if (position.CanCastleWhiteKingside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_H1, Pieces))
		{
			if (!IsInCheck(position, SQ_E1, position.GetTurn()) && !IsInCheck(position, SQ_F1, position.GetTurn()) && !IsInCheck(position, SQ_G1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_G1, KING_CASTLE));
			}
		}
	}

	if (position.CanCastleWhiteQueenside() && position.GetTurn() == WHITE)
	{
		if (mayMove(SQ_E1, SQ_A1, Pieces))
		{
			if (!IsInCheck(position, SQ_E1, position.GetTurn()) && !IsInCheck(position, SQ_D1, position.GetTurn()) && !IsInCheck(position, SQ_C1, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E1, SQ_C1, QUEEN_CASTLE));
			}
		}
	}

	if (position.CanCastleBlackKingside() && position.GetTurn() == BLACK)
	{
		if (mayMove(SQ_E8, SQ_H8, Pieces))
		{
			if (!IsInCheck(position, SQ_E8, position.GetTurn()) && !IsInCheck(position, SQ_F8, position.GetTurn()) && !IsInCheck(position, SQ_G8, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E8, SQ_G8, KING_CASTLE));
			}
		}
	}

	if (position.CanCastleBlackQueenside() && position.GetTurn() == BLACK)
	{
		if (mayMove(SQ_E8, SQ_A8, Pieces))
		{
			if (!IsInCheck(position, SQ_E8, position.GetTurn()) && !IsInCheck(position, SQ_D8, position.GetTurn()) && !IsInCheck(position, SQ_C8, position.GetTurn()))
			{
				moves.push_back(Move(SQ_E8, SQ_C8, QUEEN_CASTLE));
			}
		}
	}
}

void CalculateQuietMovesBB(const Position & position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding)
{
	assert(square < N_SQUARES);

	uint64_t quiet = position.GetEmptySquares() & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (quiet != 0)
	{
		unsigned int target = bitScanForwardErase(quiet);
		if (!isSliding || mayMove(square, target, maskall))
			moves.push_back(Move(square, target, QUIET));
	}
}

void CalculateMovesBBCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding)
{
	assert(square < N_SQUARES);

	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (captures != 0)
	{
		unsigned int target = bitScanForwardErase(captures);
		if (!isSliding || mayMove(square, target, maskall))
			moves.push_back(Move(square, target, CAPTURE));
	}
}

void RemoveIllegal(Position & position, std::vector<Move>& moves)
{
	bool turn = position.GetTurn();
	bool Pinned[64];
	unsigned int king = position.GetKing(turn);
	bool InCheck = IsInCheck(position, king, turn);
	uint64_t mask = position.GetAllPieces();

	for (int i = 0; i < 64; i++)
		Pinned[i] = InCheck;

	for (int i = 0; !InCheck && i < moves.size(); i++)
	{
		if (Pinned[moves[i].GetFrom()])										//if already pinned, skip
			continue;

		if (moves[i].GetFlag() == EN_PASSANT || moves[i].GetFrom() == king)	//En passant and any king moves must always be checked for legality
		{
			Pinned[moves[i].GetFrom()] = true;
			continue;
		}

		if (!mayMove(king, moves[i].GetFrom(), mask))						//if you can't move from the piece to the king, it can't be pinned
			continue;

		//If a piece is moving from the same diagonal as the king, and that diagonal contains an enemy bishop or queen
		if ((GetDiagonal(king) == GetDiagonal(moves[i].GetFrom())) && (GetDiagonal(king) != GetDiagonal(moves[i].GetTo())) && (DiagonalBB[GetDiagonal(king)] & (position.GetPieceBB(BISHOP, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			Pinned[moves[i].GetFrom()] = true;
			continue;
		}

		//If a piece is moving from the same anti-diagonal as the king, and that diagonal contains an enemy bishop or queen
		if ((GetAntiDiagonal(king) == GetAntiDiagonal(moves[i].GetFrom())) && (GetAntiDiagonal(king) != GetAntiDiagonal(moves[i].GetTo())) && (AntiDiagonalBB[GetAntiDiagonal(king)] & (position.GetPieceBB(BISHOP, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			Pinned[moves[i].GetFrom()] = true;
			continue;
		}

		//If a piece is moving from the same file as the king, and that file contains an enemy rook or queen
		if ((GetFile(king) == GetFile(moves[i].GetFrom())) && (GetFile(king) != GetFile(moves[i].GetTo())) && (FileBB[GetFile(king)] & (position.GetPieceBB(ROOK, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			Pinned[moves[i].GetFrom()] = true;
			continue;
		}

		//If a piece is moving from the same rank as the king, and that rank contains an enemy rook or queen
		if ((GetRank(king) == GetRank(moves[i].GetFrom())) && (GetRank(king) != GetRank(moves[i].GetTo())) && (RankBB[GetRank(king)] & (position.GetPieceBB(ROOK, !turn) | position.GetPieceBB(QUEEN, !turn))))
		{
			Pinned[moves[i].GetFrom()] = true;
			continue;
		}
	}

	for (int i = 0; i < moves.size(); i++)
	{
		if (Pinned[moves[i].GetFrom()])
		{
			if (moves[i].GetFlag() == KING_CASTLE || moves[i].GetFlag() == QUEEN_CASTLE)				//Castling moves are checked for legality at creation
				continue;

			if (MovePutsSelfInCheck(position, moves[i]))
			{
				moves.erase(moves.begin() + i);															//TODO possible speedup flag moves as illegal without actually having to delete them. 				
				i--;
			}
		}
	}
}

bool IsInCheck(const Position & position, unsigned int square, bool colour)
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

	uint64_t queen = position.GetPieceBB(QUEEN, !colour) & QueenAttacks[square];	//could put all sliding threats together
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
	return IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());
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

bool MovePutsSelfInCheck(Position & position, Move & move)
{
	unsigned int fromPiece = position.GetSquare(move.GetFrom());
	unsigned int toPiece = position.GetSquare(move.GetTo());
	unsigned int epPiece = -1;

	position.SetSquare(move.GetTo(), fromPiece);
	position.ClearSquare(move.GetFrom());

	if (move.GetFlag() == EN_PASSANT)
	{
		epPiece = position.GetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		position.ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
	}

	bool InCheck = IsInCheck(position, position.GetKing(position.GetTurn()), position.GetTurn());							//CANNOT use 'king' in place of GetKing because the king may have moved.

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
