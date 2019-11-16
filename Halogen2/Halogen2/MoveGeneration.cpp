#include "MoveGeneration.h"

void PawnPushes(const Position & position, std::vector<Move>& moves);
void PawnDoublePushes(const Position & position, std::vector<Move>& moves);
void PawnEnPassant(const Position & position, std::vector<Move>& moves);
void PawnCaptures(const Position & position, std::vector<Move>& moves);
void CastleMoves(const Position & position, std::vector<Move>& moves);

void PawnCapturesHung(const Position& position, std::vector<Move>& moves);

void CalculateMovesBB(const Position & position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding);
void CalculateMovesBBCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding);
void CalculateMovesBBHungCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding);

void RemoveIllegal(Position & position, std::vector<Move>& moves);																			//remove all the moves that put the current player's king in check

bool MovePutsSelfInCheck(Position & position, Move & move);

std::vector<Move> GenerateLegalMoves(Position & position)
{
	std::vector<Move> moves;
	GeneratePsudoLegalMoves(position, moves);
	RemoveIllegal(position, moves);

	return moves;
}

std::vector<Move> GenerateLegalCaptures(Position& position)
{
	std::vector<Move> moves;
	moves.reserve(15);

	PawnCaptures(position, moves);
	PawnEnPassant(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanFowardErase(pieces), KnightAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanFowardErase(pieces), BishopAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanFowardErase(pieces), KingAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanFowardErase(pieces), RookAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; CalculateMovesBBCapture(position, moves, bitScanFowardErase(pieces), QueenAttacks, true));

	RemoveIllegal(position, moves);
	return moves;
}

std::vector<Move> GenerateLegalHangedCaptures(Position& position)
{
	std::vector<Move> moves;
	moves.reserve(15);

	PawnCapturesHung(position, moves);
	PawnEnPassant(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; CalculateMovesBBHungCapture(position, moves, bitScanFowardErase(pieces), KnightAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; CalculateMovesBBHungCapture(position, moves, bitScanFowardErase(pieces), BishopAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; CalculateMovesBBHungCapture(position, moves, bitScanFowardErase(pieces), KingAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; CalculateMovesBBHungCapture(position, moves, bitScanFowardErase(pieces), RookAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; CalculateMovesBBHungCapture(position, moves, bitScanFowardErase(pieces), QueenAttacks, true));

	RemoveIllegal(position, moves);
	return moves;
}

void GeneratePsudoLegalMoves(const Position & position, std::vector<Move>& moves)
{
	moves.reserve(50);

	PawnPushes(position, moves);
	PawnDoublePushes(position, moves);
	PawnCaptures(position, moves);
	PawnEnPassant(position, moves);
	CastleMoves(position, moves);

	for (uint64_t pieces = position.GetPieceBB(KNIGHT, position.GetTurn()); pieces != 0; CalculateMovesBB(position, moves, bitScanFowardErase(pieces), KnightAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(KING, position.GetTurn()); pieces != 0; CalculateMovesBB(position, moves, bitScanFowardErase(pieces), KingAttacks, false));
	for (uint64_t pieces = position.GetPieceBB(BISHOP, position.GetTurn()); pieces != 0; CalculateMovesBB(position, moves, bitScanFowardErase(pieces), BishopAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(QUEEN, position.GetTurn()); pieces != 0; CalculateMovesBB(position, moves, bitScanFowardErase(pieces), QueenAttacks, true));
	for (uint64_t pieces = position.GetPieceBB(ROOK, position.GetTurn()); pieces != 0; CalculateMovesBB(position, moves, bitScanFowardErase(pieces), RookAttacks, true));
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
	uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]);			//pushes that aren't to the back ranks

	while (pawnPushes != 0)
	{
		unsigned int end = bitScanFowardErase(pawnPushes);
		moves.push_back(Move(end - foward, end, QUIET));
	}

	while (pawnPromotions != 0)
	{
		unsigned int end = bitScanFowardErase(pawnPromotions);
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
		unsigned int end = bitScanFowardErase(targets);
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
				unsigned int start = bitScanFowardErase(potentialAttackers);
				moves.push_back(Move(start, position.GetEnPassant(), EN_PASSANT));
			}
		}

		if (position.GetTurn() == BLACK)
		{
			uint64_t potentialAttackers = WhitePawnAttacks[position.GetEnPassant()] & position.GetPieceBB(BLACK_PAWN);			//if a white pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanFowardErase(potentialAttackers);
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
		unsigned int end = bitScanFowardErase(leftAttack);
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
		unsigned int end = bitScanFowardErase(rightAttack);
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

void PawnCapturesHung(const Position& position, std::vector<Move>& moves)
{
	//we have the same code here but we ignore captures of pawns because they aren't vert interesting and usually are defended anyways (doesn't improve position significantly)

	int fowardleft = 0;
	int fowardright = 0;
	uint64_t leftAttack = 0;
	uint64_t rightAttack = 0;
	uint64_t pawnSquares = position.GetPieceBB(PAWN, position.GetTurn());

	if (position.GetTurn() == WHITE)
	{
		fowardleft = 7;
		fowardright = 9;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) << 7) & position.GetBlackPieces() & ~position.GetPieceBB(BLACK_PAWN);
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) << 9) & position.GetBlackPieces() & ~position.GetPieceBB(BLACK_PAWN);
	}
	if (position.GetTurn() == BLACK)
	{
		fowardleft = -9;
		fowardright = -7;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) >> 9) & position.GetWhitePieces() & ~position.GetPieceBB(WHITE_PAWN);
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) >> 7) & position.GetWhitePieces() & ~position.GetPieceBB(WHITE_PAWN);
	}

	while (leftAttack != 0)
	{
		unsigned int end = bitScanFowardErase(leftAttack);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardleft, end, QUEEN_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardleft, end, BISHOP_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(Move(end - fowardleft, end, CAPTURE));
	}

	while (rightAttack != 0)
	{
		unsigned int end = bitScanFowardErase(rightAttack);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			moves.push_back(Move(end - fowardright, end, QUEEN_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, KNIGHT_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, ROOK_PROMOTION_CAPTURE));
			moves.push_back(Move(end - fowardright, end, BISHOP_PROMOTION_CAPTURE));
		}
		else
			moves.push_back(Move(end - fowardright, end, CAPTURE));
	}
}

void CalculateMovesBB(const Position & position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding)
{
	if (square >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");

	uint64_t quiet = position.GetEmptySquares() & attackMask[square];
	uint64_t captures = position.GetPiecesColour(!position.GetTurn()) & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (quiet != 0)
	{
		unsigned int target = bitScanFowardErase(quiet);
		if (!isSliding || mayMove(square, target, maskall))
			moves.push_back(Move(square, target, QUIET));
	}

	while (captures != 0)
	{
		unsigned int target = bitScanFowardErase(captures);
		if (!isSliding || mayMove(square, target, maskall))
			moves.push_back(Move(square, target, CAPTURE));
	}
}

void CalculateMovesBBCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding)
{
	if (square >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");

	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (captures != 0)
	{
		unsigned int target = bitScanFowardErase(captures);
		if (!isSliding || mayMove(square, target, maskall))
			moves.push_back(Move(square, target, CAPTURE));
	}
}

void CalculateMovesBBHungCapture(const Position& position, std::vector<Move>& moves, unsigned int square, uint64_t attackMask[N_SQUARES], bool isSliding)
{
	if (square >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");

	uint64_t captures = (position.GetPiecesColour(!position.GetTurn())) & attackMask[square];
	uint64_t maskall = position.GetAllPieces() & attackMask[square];

	while (captures != 0)
	{
		unsigned int target = bitScanFowardErase(captures);
		if ((!isSliding || mayMove(square, target, maskall)) && !IsInCheck(position, target, position.GetTurn()))	//if I would be in check at target, then it is a defended piece so ignore.
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
	if (square >= N_SQUARES) throw std::invalid_argument("Bad paramiter to SetSquare()");

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
		unsigned int start = bitScanFowardErase(queen);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t bishops = position.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
	while (bishops != 0)
	{
		unsigned int start = bitScanFowardErase(bishops);
		if (mayMove(start, square, Pieces))
			return true;
	}

	uint64_t rook = position.GetPieceBB(ROOK, !colour) & RookAttacks[square];
	while (rook != 0)
	{
		unsigned int start = bitScanFowardErase(rook);
		if (mayMove(start, square, Pieces))
			return true;
	}

	return false;
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
