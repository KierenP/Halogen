#include "Position.h"

void Position::PawnPushes(std::vector<Move>& moves)
{
	int foward = 0;
	uint64_t targets = 0;
	uint64_t pawnSquares = GetPieces(Piece(PAWN, BoardParamiter.CurrentTurn));

	if (BoardParamiter.CurrentTurn == WHITE)
	{
		foward = 8;
		targets = (pawnSquares << 8) & EmptySquares();
	}
	if (BoardParamiter.CurrentTurn == BLACK)
	{
		foward = -8;
		targets = (pawnSquares >> 8) & EmptySquares();
	}

	while (targets != 0)
	{
		unsigned int end = bitScanFowardErase(targets);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			AddMoves(end - foward, end, KNIGHT_PROMOTION, moves);
			AddMoves(end - foward, end, ROOK_PROMOTION, moves);
			AddMoves(end - foward, end, BISHOP_PROMOTION, moves);
			AddMoves(end - foward, end, QUEEN_PROMOTION, moves);
		}
		else
			AddMoves(end - foward, end, QUIET, moves);
	}
}

void Position::PawnDoublePushes(std::vector<Move>& moves)
{
	int foward = 0;
	uint64_t targets = 0;
	uint64_t pawnSquares = GetPieces(Piece(PAWN, BoardParamiter.CurrentTurn));

	if (BoardParamiter.CurrentTurn == WHITE)
	{
		foward = 16;
		pawnSquares &= RankBB[RANK_2];
		targets = (pawnSquares << 8) & EmptySquares();
		targets = (targets << 8) & EmptySquares();
	}
	if (BoardParamiter.CurrentTurn == BLACK)
	{
		foward = -16;
		pawnSquares &= RankBB[RANK_7];
		targets = (pawnSquares >> 8) & EmptySquares();
		targets = (targets >> 8) & EmptySquares();
	}

	while (targets != 0)
	{
		unsigned int end = bitScanFowardErase(targets);
		AddMoves(end - foward, end, PAWN_DOUBLE_MOVE, moves);
	}
}

void Position::PawnAttacks(std::vector<Move>& moves)
{
	int fowardleft = 0;
	int fowardright = 0;
	uint64_t leftAttack = 0;			
	uint64_t rightAttack = 0;			
	uint64_t pawnSquares = GetPieces(Piece(PAWN, BoardParamiter.CurrentTurn));

	if (BoardParamiter.CurrentTurn == WHITE)
	{
		fowardleft = 7;
		fowardright = 9;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) << 7) & BlackPieces();
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) << 9) & BlackPieces();
	}
	if (BoardParamiter.CurrentTurn == BLACK)
	{
		fowardleft = -9;
		fowardright = -7;
		leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) >> 9) & WhitePieces();
		rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) >> 7) & WhitePieces();
	}

	while (leftAttack != 0)
	{
		unsigned int end = bitScanFowardErase(leftAttack);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			AddMoves(end - fowardleft, end, KNIGHT_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardleft, end, ROOK_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardleft, end, BISHOP_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardleft, end, QUEEN_PROMOTION_CAPTURE, moves);
		}
		else
			AddMoves(end - fowardleft, end, CAPTURE, moves);
	}

	while (rightAttack != 0)
	{
		unsigned int end = bitScanFowardErase(rightAttack);
		if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
		{
			AddMoves(end - fowardright, end, KNIGHT_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardright, end, ROOK_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardright, end, BISHOP_PROMOTION_CAPTURE, moves);
			AddMoves(end - fowardright, end, QUEEN_PROMOTION_CAPTURE, moves);
		}
		else
			AddMoves(end - fowardright, end, CAPTURE, moves);
	}

	if (BoardParamiter.EnPassant <= SQ_H8 && BoardParamiter.EnPassant >= SQ_A1)
	{
		if (BoardParamiter.CurrentTurn == WHITE)
		{
			uint64_t potentialAttackers = BlackPawnAttacks[BoardParamiter.EnPassant] & GetPieces(WHITE_PAWN);			//if a black pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanFowardErase(potentialAttackers);
				AddMoves(start, BoardParamiter.EnPassant, EN_PASSANT, moves);
			}
		}

		if (BoardParamiter.CurrentTurn == BLACK)
		{
			uint64_t potentialAttackers = WhitePawnAttacks[BoardParamiter.EnPassant] & GetPieces(BLACK_PAWN);			//if a white pawn could capture me from the ep square, I can capture on the ep square
			while (potentialAttackers != 0)
			{
				unsigned int start = bitScanFowardErase(potentialAttackers);
				AddMoves(start, BoardParamiter.EnPassant, EN_PASSANT, moves);
			}
		}
	}

	/*uint64_t pawnSquares = GetPieces(Piece(PAWN, BoardParamiter.CurrentTurn));

	if (BoardParamiter.CurrentTurn == WHITE)
	{
		while (pawnSquares != 0)
		{
			unsigned int start = bitScanFowardErase(pawnSquares);

			if (Occupied(start + 7, BLACK) && GetFile(start) != FILE_A)
				AddMoves(start, start + 7, CAPTURE, moves);
			if (Occupied(start + 9, BLACK) && GetFile(start) != FILE_H)
				AddMoves(start, start + 9, CAPTURE, moves);
			if (start + 7 == BoardParamiter.EnPassant && GetFile(start) != FILE_A)
				AddMoves(start, start + 7, EN_PASSANT, moves);
			if (start + 9 == BoardParamiter.EnPassant && GetFile(start) != FILE_H)
				AddMoves(start, start + 9, EN_PASSANT, moves);
		}
	}
	if (BoardParamiter.CurrentTurn == BLACK)
	{
		while (pawnSquares != 0)
		{
			unsigned int start = bitScanFowardErase(pawnSquares);

			if (Occupied(start - 7, WHITE) && GetFile(start) != FILE_H)
				AddMoves(start, start - 7, CAPTURE, moves);
			if (Occupied(start - 9, WHITE) && GetFile(start) != FILE_A)
				AddMoves(start, start - 9, CAPTURE, moves);
			if (start - 7 == BoardParamiter.EnPassant && GetFile(start) != FILE_H)
				AddMoves(start, start - 7, EN_PASSANT, moves);
			if (start - 9 == BoardParamiter.EnPassant && GetFile(start) != FILE_A)
				AddMoves(start, start - 9, EN_PASSANT, moves);
		}
	}*/
}

void Position::KnightMoves(std::vector<Move>& moves)
{
	uint64_t KnightSquares = GetPieces(Piece(KNIGHT, BoardParamiter.CurrentTurn));

	while (KnightSquares != 0)
	{
		unsigned int start = bitScanFowardErase(KnightSquares);
		uint64_t quietbb = EmptySquares() & KnightAttacks[start];
		uint64_t capturebb = ColourPieces(!BoardParamiter.CurrentTurn) & KnightAttacks[start];

		while (quietbb != 0)
		{
			unsigned int end = bitScanFowardErase(quietbb);
			AddMoves(start, end, QUIET, moves);
		}	

		while (capturebb != 0)
		{
			unsigned int end = bitScanFowardErase(capturebb);
			AddMoves(start, end, CAPTURE, moves);
		}	
	}
}

void Position::SlidingMovesBB(std::vector<Move>& moves, uint64_t movePiece, uint64_t attackMask[N_SQUARES])
{
	//This is called by every rook bishop and queen move and hence any speedup here is very benifitial

	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t quiet = EmptySquares() & attackMask[start];
		uint64_t captures = ColourPieces(!BoardParamiter.CurrentTurn) & attackMask[start];
		uint64_t maskall = AllPieces() & attackMask[start];

		while (quiet != 0)
		{
			unsigned int sq = bitScanFowardErase(quiet);
			if (mayMove(start, sq, maskall))
				moves.push_back(Move(start, sq, QUIET));
		}

		while (captures != 0)
		{
			unsigned int sq = bitScanFowardErase(captures);
			if (mayMove(start, sq, maskall))
				moves.push_back(Move(start, sq, CAPTURE));
		}
	}
}

void Position::RookMoves(std::vector<Move>& moves)
{
	SlidingMovesBB(moves, GetPieces(Piece(ROOK, BoardParamiter.CurrentTurn)), RookAttacks);
}

void Position::BishopMoves(std::vector<Move>& moves)
{
	SlidingMovesBB(moves, GetPieces(Piece(BISHOP, BoardParamiter.CurrentTurn)), BishopAttacks);
}

void Position::QueenMoves(std::vector<Move>& moves)
{
	SlidingMovesBB(moves, GetPieces(Piece(QUEEN, BoardParamiter.CurrentTurn)), QueenAttacks);
}

void Position::KingMoves(std::vector<Move>& moves)
{
	uint64_t KingSquares = GetPieces(Piece(KING, BoardParamiter.CurrentTurn));

	while (KingSquares != 0)
	{
		unsigned int start = bitScanFowardErase(KingSquares);
		uint64_t quietbb = EmptySquares() & KingAttacks[start];
		uint64_t capturebb = ColourPieces(!BoardParamiter.CurrentTurn) & KingAttacks[start];

		while (quietbb != 0)
		{
			unsigned int end = bitScanFowardErase(quietbb);
			AddMoves(start, end, QUIET, moves);
		}

		while (capturebb != 0)
		{
			unsigned int end = bitScanFowardErase(capturebb);
			AddMoves(start, end, CAPTURE, moves);
		}
	}

	/*int offset[8] = { -9, -8, -7, -1, 1, 7, 8, 9 };
	uint64_t Squares = GetPieces(Piece(KING, BoardParamiter.CurrentTurn));

	while (Squares != 0)
	{
		unsigned int start = bitScanFowardErase(Squares);
		SlidingMoves(start, offset, false, BoardParamiter.CurrentTurn, moves);
	}*/
}

void Position::CastleMoves(std::vector<Move>& moves)
{
	uint64_t Pieces = AllPieces();

	if (BoardParamiter.WhiteKingCastle && BoardParamiter.CurrentTurn == WHITE)
	{
		if (mayMove(SQ_E1, SQ_H1, Pieces))
		{
			if (!IsInCheck(SQ_E1, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_F1, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_G1, BoardParamiter.CurrentTurn))
			{
				AddMoves(SQ_E1, SQ_G1, KING_CASTLE, moves);
			}
		}
	}

	if (BoardParamiter.WhiteQueenCastle && BoardParamiter.CurrentTurn == WHITE)
	{
		if (mayMove(SQ_E1, SQ_A1, Pieces))
		{
			if (!IsInCheck(SQ_E1, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_D1, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_C1, BoardParamiter.CurrentTurn))
			{
				AddMoves(SQ_E1, SQ_C1, QUEEN_CASTLE, moves);
			}
		}
	}

	if (BoardParamiter.BlackKingCastle && BoardParamiter.CurrentTurn == BLACK)
	{
		if (mayMove(SQ_E8, SQ_H8, Pieces))
		{
			if (!IsInCheck(SQ_E8, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_F8, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_G8, BoardParamiter.CurrentTurn))
			{
				AddMoves(SQ_E8, SQ_G8, KING_CASTLE, moves);
			}
		}
	}

	if (BoardParamiter.BlackQueenCastle && BoardParamiter.CurrentTurn == BLACK)
	{
		if (mayMove(SQ_E8, SQ_A8, Pieces))
		{
			if (!IsInCheck(SQ_E8, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_D8, BoardParamiter.CurrentTurn) && !IsInCheck(SQ_C8, BoardParamiter.CurrentTurn))
			{
				AddMoves(SQ_E8, SQ_C8, QUEEN_CASTLE, moves);
			}
		}
	}
}

void Position::PromotionMoves(std::vector<Move>& moves)
{
	for (int i = 0; i < moves.size(); i++)
	{
		if (((GetSquare(moves[i].GetFrom()) == WHITE_PAWN && GetRank(moves[i].GetTo()) == RANK_8) ||
			(GetSquare(moves[i].GetFrom()) == BLACK_PAWN && GetRank(moves[i].GetTo()) == RANK_1)) && !moves[i].IsPromotion())
		{
			if (moves[i].GetFlag() == QUIET)		
			{
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), KNIGHT_PROMOTION, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), ROOK_PROMOTION, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), BISHOP_PROMOTION, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), QUEEN_PROMOTION, moves);
			}
			else
			{
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), KNIGHT_PROMOTION_CAPTURE, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), ROOK_PROMOTION_CAPTURE, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), BISHOP_PROMOTION_CAPTURE, moves);
				AddMoves(moves[i].GetFrom(), moves[i].GetTo(), QUEEN_PROMOTION_CAPTURE, moves);
			}
			moves.erase(moves.begin() + i);												//erase the pawn push
			i--;
		}
	}
}

void Position::GenerateAttackTables(uint64_t(&table)[N_PIECES])
{
	for (int i = 0; i < N_PIECES; i++)
	{
		table[i] = EMPTY;
	}

	uint64_t mask = AllPieces();

	uint64_t movePiece = GetPieces(WHITE_KNIGHT);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KnightAttacks[start] & (~WhitePieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			table[WHITE_KNIGHT] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(BLACK_KNIGHT);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = KnightAttacks[start] & (~BlackPieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			table[BLACK_KNIGHT] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(WHITE_PAWN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = WhitePawnAttacks[start] & (~WhitePieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			table[WHITE_PAWN] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(BLACK_PAWN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BlackPawnAttacks[start] & (~BlackPieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			table[BLACK_PAWN] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(WHITE_BISHOP);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BishopAttacks[start] & (~WhitePieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[WHITE_BISHOP] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(BLACK_BISHOP);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = BishopAttacks[start] & (~BlackPieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[BLACK_BISHOP] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(WHITE_ROOK);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = RookAttacks[start] & (~WhitePieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[WHITE_ROOK] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(BLACK_ROOK);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = RookAttacks[start] & (~BlackPieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[BLACK_ROOK] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(WHITE_QUEEN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = QueenAttacks[start] & (~WhitePieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[WHITE_QUEEN] |= SquareBB[sq];
		}
	}

	movePiece = GetPieces(BLACK_QUEEN);
	while (movePiece != 0)
	{
		unsigned int start = bitScanFowardErase(movePiece);
		uint64_t moves = QueenAttacks[start] & (~BlackPieces());

		while (moves != 0)
		{
			unsigned int sq = bitScanFowardErase(moves);
			if (mayMove(start, sq, mask))
				table[BLACK_QUEEN] |= SquareBB[sq];
		}
	}
}