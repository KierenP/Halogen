#include "Position.h"

uint64_t PreviousPositions[1000];
PositionParam PreviousParam[1000];

Position::Position()
{
	BBInit();
	EvalInit();

	//store knight attacks as pre computed.
	for (int i = 0; i < 64; i++)
	{
		for (int j = 0; j < 64; j++)
		{
			if ((RankDiff(i, j) == 1 && FileDiff(i, j) == 2) || (RankDiff(i, j) == 2 && FileDiff(i, j) == 1))
				KnightAttacks[i] |= SquareBB[j];
		}
	}

	for (int i = 0; i < 1000; i++)
	{
		PreviousPositions[i] = EMPTY;
	}
}


Position::~Position()
{
}

bool Position::InitialiseFromFen(std::string board, std::string turn, std::string castle, std::string ep, std::string fiftyMove, std::string turnCount)
{
	Reset();

	int i = 0;									//index within the string
	int j = 0;									//index within the board
	while ((j < 64) && (i < board.length()))
	{
		char letter = board.at(i);
		int sq = GetPosition(GetFile(j), 7 - GetRank(j));

		switch (letter)
		{
		case 'p': SetSquare(sq, BLACK_PAWN); break;
		case 'r': SetSquare(sq, BLACK_ROOK); break;
		case 'n': SetSquare(sq, BLACK_KNIGHT); break;
		case 'b': SetSquare(sq, BLACK_BISHOP); break;
		case 'q': SetSquare(sq, BLACK_QUEEN); break;
		case 'k': SetSquare(sq, BLACK_KING); break;
		case 'P': SetSquare(sq, WHITE_PAWN); break;
		case 'R': SetSquare(sq, WHITE_ROOK); break;
		case 'N': SetSquare(sq, WHITE_KNIGHT); break;
		case 'B': SetSquare(sq, WHITE_BISHOP); break;
		case 'Q': SetSquare(sq, WHITE_QUEEN); break;
		case 'K': SetSquare(sq, WHITE_KING); break;
		case '/': j--; break;
		case '1': break;
		case '2': j += 1; break;
		case '3': j += 2; break;
		case '4': j += 3; break;
		case '5': j += 4; break;
		case '6': j += 5; break;
		case '7': j += 6; break;
		case '8': j += 7; break;
		default: return false;					//bad fen
		}
		j++;
		i++;
	}

	BoardParamiter.WhiteKingCastle = false;
	BoardParamiter.WhiteQueenCastle = false;
	BoardParamiter.BlackKingCastle = false;
	BoardParamiter.BlackQueenCastle = false;


	if (turn == "w")
		BoardParamiter.CurrentTurn = WHITE;

	if (turn == "b")
		BoardParamiter.CurrentTurn = BLACK;

	if (castle.find('K') != std::string::npos)
		BoardParamiter.WhiteKingCastle = true;

	if (castle.find('Q') != std::string::npos)
		BoardParamiter.WhiteQueenCastle = true;

	if (castle.find('k') != std::string::npos)
		BoardParamiter.BlackKingCastle = true;

	if (castle.find('q') != std::string::npos)
		BoardParamiter.BlackQueenCastle = true;

	BoardParamiter.EnPassant = AlgebraicToPos(ep);

	BoardParamiter.FiftyMoveCount = std::stoi(fiftyMove);
	BoardParamiter.TurnCount = std::stoi(turnCount);
	CalculateGameStage();

	return true;
}

void Position::Print()
{
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hConsole, 10);	//green text
	std::cout << "\n  A B C D E F G H";
	SetConsoleTextAttribute(hConsole, 7);	//back to gray

	char Letter[N_SQUARES];
	unsigned int colour[N_SQUARES];

	for (int i = 0; i < N_SQUARES; i++)
	{
		Letter[i] = PieceToChar(GetSquare(i));
		colour[i] = 7;									//grey
	}

	for (int i = 0; i < N_SQUARES; i++)
	{
		unsigned int square = GetPosition(GetFile(i), 7 - GetRank(i));		//7- to flip on the y axis and do rank 8, 7 ... 

		if (GetFile(square) == FILE_A)
		{
			std::cout << std::endl;									//Go to a new line
			SetConsoleTextAttribute(hConsole, 10);					//print the number green
			std::cout << 8 - GetRank(i);							//Count down from 8
		}

		std::cout << " ";

		SetConsoleTextAttribute(hConsole, colour[square]);			//Set colour to that squares colour
		std::cout << Letter[square];
	}

	SetConsoleTextAttribute(hConsole, 7);							//and back to gray

	std::cout << std::endl;
}

void Position::ApplyMove(Move & move)
{
	//PositionParam* oldparam = new PositionParam(BoardParamiter);		//make a copy of the old
	//BoardParamiter.Prev = oldparam;										//set current's previous to that copy
	PreviousParam[BoardParamiter.TurnCount] = BoardParamiter;

	BoardParamiter.EnPassant = -1;
	BoardParamiter.CaptureSquare = -1;

	switch (move.GetFlag())
	{
	case QUIET:
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case PAWN_DOUBLE_MOVE:
		BoardParamiter.EnPassant = (move.GetTo() + move.GetFrom()) / 2;			//average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white 
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case KING_CASTLE:
		if (BoardParamiter.CurrentTurn == WHITE)
			BoardParamiter.HasCastledWhite = true;
		else
			BoardParamiter.HasCastledBlack = true;
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		SetSquare(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_H, GetRank(move.GetFrom())));
		break;
	case QUEEN_CASTLE:
		if (BoardParamiter.CurrentTurn == WHITE)
			BoardParamiter.HasCastledWhite = true;
		else
			BoardParamiter.HasCastledBlack = true;
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		SetSquare(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))));
		ClearSquare(GetPosition(FILE_A, GetRank(move.GetFrom())));
		break;
	case CAPTURE:
		BoardParamiter.CapturePiece = GetSquare(move.GetTo());
		BoardParamiter.CaptureSquare = move.GetTo();
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		break;
	case EN_PASSANT:
		BoardParamiter.CapturePiece = GetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		BoardParamiter.CaptureSquare = GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()));
		SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
		ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
		break;
	case KNIGHT_PROMOTION:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		SetSquare(move.GetTo(), Piece(KNIGHT, BoardParamiter.CurrentTurn));
		break;
	case BISHOP_PROMOTION:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		SetSquare(move.GetTo(), Piece(BISHOP, BoardParamiter.CurrentTurn));
		break;
	case ROOK_PROMOTION:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		SetSquare(move.GetTo(), Piece(ROOK, BoardParamiter.CurrentTurn));
		break;
	case QUEEN_PROMOTION:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		SetSquare(move.GetTo(), Piece(QUEEN, BoardParamiter.CurrentTurn));
		break;
	case KNIGHT_PROMOTION_CAPTURE:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		BoardParamiter.CapturePiece = GetSquare(move.GetTo());
		SetSquare(move.GetTo(), Piece(KNIGHT, BoardParamiter.CurrentTurn));
		break;
	case BISHOP_PROMOTION_CAPTURE:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		BoardParamiter.CapturePiece = GetSquare(move.GetTo());
		SetSquare(move.GetTo(), Piece(BISHOP, BoardParamiter.CurrentTurn));
		break;
	case ROOK_PROMOTION_CAPTURE:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		BoardParamiter.CapturePiece = GetSquare(move.GetTo());
		SetSquare(move.GetTo(), Piece(ROOK, BoardParamiter.CurrentTurn));
		break;
	case QUEEN_PROMOTION_CAPTURE:
		BoardParamiter.PromotionPiece = GetSquare(move.GetFrom());
		BoardParamiter.CapturePiece = GetSquare(move.GetTo());
		SetSquare(move.GetTo(), Piece(QUEEN, BoardParamiter.CurrentTurn));
		break;
	default:
		break;
	}

	ClearSquare(move.GetFrom());

	BoardParamiter.TurnCount++;
	BoardParamiter.GameStage = CalculateGameStage();

	UpdateCastleRights(move);
	BoardParamiter.CurrentTurn = !BoardParamiter.CurrentTurn;
	PreviousPositions[BoardParamiter.TurnCount] = GenerateZobristKey();
}

void Position::ApplyMove(std::string strmove)
{
	unsigned int prev = (strmove[0] - 97) + (strmove[1] - 49) * 8;
	unsigned int next = (strmove[2] - 97) + (strmove[3] - 49) * 8;

	//Defult Quiet
	Move move(prev, next, QUIET);

	//Captures
	if (Occupied(next))
		move.SetFlag(CAPTURE);

	//Double pawn moves
	if (RankDiff(prev, next) == 2)
	{
		if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
		{
			move.SetFlag(PAWN_DOUBLE_MOVE);
		}
	}

	//En passant
	if (next == BoardParamiter.EnPassant)
	{
		if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
			move.SetFlag(EN_PASSANT);
	}

	//Castling
	if (prev == SQ_E1 && next == SQ_G1 && GetSquare(prev) == WHITE_KING)
	{
		move.SetFlag(KING_CASTLE);
	}

	if (prev == SQ_E1 && next == SQ_C1 && GetSquare(prev) == WHITE_KING)
	{
		move.SetFlag(QUEEN_CASTLE);
	}

	if (prev == SQ_E8 && next == SQ_G8 && GetSquare(prev) == BLACK_KING)
	{
		move.SetFlag(KING_CASTLE);
	}

	if (prev == SQ_E8 && next == SQ_C8 && GetSquare(prev) == BLACK_KING)
	{
		move.SetFlag(QUEEN_CASTLE);
	}

	//Promotion
	if (strmove.length() == 5)		//promotion: c7c8q or d2d1n	etc.
	{
		if (move.IsCapture())
		{
			if (tolower(strmove[4]) == 'n')
				move.SetFlag(KNIGHT_PROMOTION_CAPTURE);

			if (tolower(strmove[4]) == 'r')
				move.SetFlag(ROOK_PROMOTION_CAPTURE);

			if (tolower(strmove[4]) == 'q')
				move.SetFlag(QUEEN_PROMOTION_CAPTURE);

			if (tolower(strmove[4]) == 'b')
				move.SetFlag(BISHOP_PROMOTION_CAPTURE);
		}
		else
		{
			if (tolower(strmove[4]) == 'n')
				move.SetFlag(KNIGHT_PROMOTION);

			if (tolower(strmove[4]) == 'r')
				move.SetFlag(ROOK_PROMOTION);

			if (tolower(strmove[4]) == 'q')
				move.SetFlag(QUEEN_PROMOTION);

			if (tolower(strmove[4]) == 'b')
				move.SetFlag(BISHOP_PROMOTION);
		}
	}

	ApplyMove(move);
}

void Position::RevertMove(Move & move)
{
	PreviousPositions[BoardParamiter.TurnCount] = EMPTY;
	//PreviousParam[BoardParamiter.TurnCount] = PositionParam();

	SetSquare(move.GetFrom(), GetSquare(move.GetTo()));
	ClearSquare(move.GetTo());

	switch (move.GetFlag())
	{
	case QUIET:
		break;
	case PAWN_DOUBLE_MOVE:
		break;
	case KING_CASTLE:
		SetSquare(GetPosition(FILE_H, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_F, GetRank(move.GetFrom()))));		//move the rook back
		ClearSquare(GetPosition(FILE_F, GetRank(move.GetFrom())));
		break;
	case QUEEN_CASTLE:
		SetSquare(GetPosition(FILE_A, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_D, GetRank(move.GetFrom()))));		//move the rook back
		ClearSquare(GetPosition(FILE_D, GetRank(move.GetFrom())));
		break;
	case CAPTURE:
		SetSquare(move.GetTo(), BoardParamiter.CapturePiece);
		break;
	case EN_PASSANT:
		SetSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())), BoardParamiter.CapturePiece);
		break;
	case KNIGHT_PROMOTION:
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case BISHOP_PROMOTION:
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case ROOK_PROMOTION:
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case QUEEN_PROMOTION:
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case KNIGHT_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), BoardParamiter.CapturePiece);
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case BISHOP_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), BoardParamiter.CapturePiece);
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case ROOK_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), BoardParamiter.CapturePiece);
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	case QUEEN_PROMOTION_CAPTURE:
		SetSquare(move.GetTo(), BoardParamiter.CapturePiece);
		SetSquare(move.GetFrom(), BoardParamiter.PromotionPiece);
		break;
	default:
		break;
	}

	//PositionParam* temp = BoardParamiter.Prev;
	//BoardParamiter = *BoardParamiter.Prev;
	//delete temp;

	//UpdateCastleRights(move);

	BoardParamiter.TurnCount--;
	BoardParamiter = PreviousParam[BoardParamiter.TurnCount];			//make it equal to what came before it
}

std::vector<Move>& Position::GenerateLegalMoves()
{
	RemoveIllegal(GeneratePsudoLegalMoves());
	return LegalMoves;
}

std::vector<Move>& Position::GeneratePsudoLegalMoves()
{
	LegalMoves.clear();

	CastleMoves(LegalMoves);
	KnightMoves(LegalMoves);
	BishopMoves(LegalMoves);
	QueenMoves(LegalMoves);
	RookMoves(LegalMoves);
	PawnPushes(LegalMoves);
	PawnAttacks(LegalMoves);
	PawnDoublePushes(LegalMoves);
	KingMoves(LegalMoves);

	return LegalMoves;
}

bool Position::GetCurrentTurn()
{
	return BoardParamiter.CurrentTurn;
}

void Position::SetCurrentTurn(bool turn)
{
	BoardParamiter.CurrentTurn = turn;
}

double Position::Perft(int depth)
{
	perftTable.Reformat();

	double nodeCount = 0;

	GeneratePsudoLegalMoves();

	if (depth == 1)
	{
		RemoveIllegal(LegalMoves);
		return LegalMoves.size();
	}

	std::vector<Move> moves = LegalMoves;

	for (int i = 0; i < moves.size(); i++)
	{
		ApplyMove(moves[i]);

		if (IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn))
		{
			RevertMove(moves[i]);
			continue;					//Illegal move
		}

		unsigned int ChildNodeCount = 0;
		uint64_t key = GenerateZobristKey();
		if ((perftTable.CheckEntry(key)) && (perftTable.GetEntry(key).GetDepth() == depth - 1))
		{
			ChildNodeCount = perftTable.GetEntry(key).GetNodes();
		}
		else
		{
			ChildNodeCount = PerftLeaf(depth - 1);
			perftTable.AddEntry(key, ChildNodeCount, depth -1);
		}
		//unsigned int ChildNodeCount = PerftLeaf(depth - 1);
		RevertMove(moves[i]);

		//moves[i].Print();
		//std::cout << ": " << ChildNodeCount << std::endl;
		nodeCount += ChildNodeCount;
	}

	return nodeCount;
}

/*int Position::pvs(ABnode * node, int depth, int alpha, int beta, bool colour)
{
	if (depth == 0)
	{
		node->SetScore(Evaluate());
		node->SetCutoff(EXACT);
		return node->GetScore();
	}

	GenerateLegalMoves();

	if (LegalMoves.size() == 0)
	{
		node->SetScore(Evaluate());
		node->SetCutoff(EXACT);
		return node->GetScore();
	}

	MoveOrder(LegalMoves);
	std::vector<Move> moves = LegalMoves;

	for (int i = 0; i < moves.size(); i++)
	{
		ABnode* child = new ABnode();
		child->SetMove(moves[i]);
		child->SetDepth(depth);
		child->SetParent(node);

		ApplyMove(moves[i]);

		if (i == 0)
		{
			child->SetScore(-pvs(child, depth - 1, -beta, -alpha, !colour));
			if (node->HasChild())
				delete node->GetChild();
			node->SetChild(child);
		}
		else
		{
			child->SetScore(-pvs(child, depth - 1, -alpha-1, -alpha, !colour));
			if (alpha < child->GetScore() && child->GetScore() < beta)
				child->SetScore(-pvs(child, depth - 1, -beta, -child->GetScore(), !colour));
		}

		if (child->GetScore() >= alpha)
		{
			if (node->HasChild())
				delete node->GetChild();
			node->SetChild(child);
			alpha = child->GetScore();
		}
		if (alpha >= beta)
		{
			if (node->HasChild())
				delete node->GetChild();
			node->SetChild(child);
			break;				//beta cutoff
		}

		if (node->GetChild() != child)
			delete child;

		RevertMove(moves[i]);
	}

	return alpha;
}*/

int Position::BestMove(int depth, int alpha, int beta, ABnode* parent, Move prevBest, double AllowedTime)
{
	SYSTEMTIME before;
	SYSTEMTIME after;

	double PassedTime = 0;

	GenerateLegalMoves();
	NodeCount = 0;

	if (LegalMoves.size() == 1)										//with only one move, we can very quickly just return this move as there is zero benifet to searching the tree below this node
	{
		ABnode* node = new ABnode(NONE, LegalMoves[0], depth);
		node->SetParent(parent);
		parent->SetCutoff(NONE);
		parent->SetChild(node);
		return 0;
	}

	MoveOrder(LegalMoves, depth);
	std::vector<Move> moves = LegalMoves;
	bool turn = BoardParamiter.CurrentTurn;
	ABnode* best = new ABnode(NONE, Move(0, 0, 0), depth, -1);

	for (int i = 0; i < moves.size(); i++)	//This SHOULD increace preformance with iterative deepening
	{
		if (moves[i].GetTo() == prevBest.GetTo() && moves[i].GetFrom() == prevBest.GetFrom() && moves[i].GetFlag() == prevBest.GetFlag())
		{
			Move temp = moves[i];
			moves[i] = moves[0];
			moves[0] = temp;				//move it to the front
		}
	}

	for (int i = 0; i < moves.size(); i++)
	{
		GetSystemTime(&before);

		ABnode* node = new ABnode(NONE, moves[i], depth);

		ApplyMove(moves[i]);
		if (turn == BLACK)
			node->SetScore(NegaAlphaBetaMax(node, depth - 1, alpha, beta, 1, true));
		if (turn == WHITE)
			node->SetScore(-NegaAlphaBetaMax(node, depth - 1, alpha, beta, -1, true));
		RevertMove(moves[i]);

		if (best->GetScore() == -1 || ((node->GetScore() > best->GetScore() && turn == WHITE) || (node->GetScore() < best->GetScore() && turn == BLACK)))
		{
			delete best;
			best = node;
		}
		else
			delete node;

		GetSystemTime(&after);
		double Time = after.wDay * 1000 * 60 * 60 * 24 + after.wHour * 60 * 60 * 1000 + after.wMinute * 60 * 1000 + after.wSecond * 1000 + after.wMilliseconds - before.wDay * 1000 * 60 * 60 * 24 - before.wHour * 60 * 60 * 1000 - before.wMinute * 60 * 1000 - before.wSecond * 1000 - before.wMilliseconds;
		PassedTime += Time;

		if ((Time / (i + 1)) * moves.size() > AllowedTime * 1.5 && (Time / (i + 1)) > 1000)
			return -2;
	}

	best->SetParent(parent);
	parent->SetCutoff(best->GetCutoff());
	parent->SetChild(best);
	return best->GetScore();
}

void Position::ResetTT()
{
	tTable.Reformat();
	tTable.TTHits = 0;
}

bool Position::CheckThreeFold()
{
	unsigned int same = 0;
	uint64_t key = GenerateZobristKey();
	for (int i = 0; i < BoardParamiter.TurnCount; i++)
	{
		if (PreviousPositions[i] == key)
			same++;
	}

	if (same >= 2)
		return true;			//draw by 3 fold rep
	return false;
}

void Position::StartingPosition()
{
	InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
}

unsigned int Position::GetGameState()
{
	return BoardParamiter.GameStage;
}

unsigned int Position::CalculateGameStage()
{
	if (BoardParamiter.TurnCount < 10)
		return OPENING;

	if (BoardParamiter.TurnCount > 25)
		return ENDGAME;

	return MIDGAME;
}

std::vector<Move>& Position::MoveOrder(std::vector<Move>& moves, unsigned int depth)
{
	unsigned int swapIndex = 0;
	Move temp;
	int PieceValues[N_PIECES] = { 1, 3, 3, 5, 9, 20, 1, 3, 3, 5, 9, 20 };			//relative values

	//Move hashed move to front
	if (tTable.CheckEntry(GenerateZobristKey(), depth - 1))
	{
		Move bestprev = tTable.GetEntry(GenerateZobristKey()).GetMove();
		for (int i = 0; i < moves.size(); i++)
		{
			if (bestprev.GetFrom() == moves[i].GetFrom() && bestprev.GetTo() == moves[i].GetTo() && bestprev.GetFlag() == moves[i].GetFlag())
			{
				temp = moves[i];
				moves[i] = moves[0];
				moves[0] = temp;

				swapIndex++;
				break;
			}
		}
	}

	//Move promotions to front
	for (int i = swapIndex; i < moves.size(); i++)
	{
		if (moves[i].IsPromotion())
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;
			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		for (int j = 20; j <= 0; j--)
		{
			if (moves[i].IsCapture() && (PieceValues[GetSquare(moves[i].GetTo())] - PieceValues[GetSquare(moves[i].GetFrom())] == j))
			{
				temp = moves[i];
				moves[i] = moves[swapIndex];
				moves[swapIndex] = temp;
				swapIndex++;
			}
		}
	}

	/*
	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(KING, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;
			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(QUEEN, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(ROOK, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(BISHOP, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(KNIGHT, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].IsCapture()) && (GetSquare(moves[i].GetTo()) == Piece(PAWN, !BoardParamiter.CurrentTurn)))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}

	for (int i = swapIndex; i < moves.size(); i++)
	{
		if ((moves[i].GetFlag() == KING_CASTLE) || (moves[i].GetFlag() == QUEEN_CASTLE))
		{
			temp = moves[i];
			moves[i] = moves[swapIndex];
			moves[swapIndex] = temp;

			swapIndex++;
		}
	}*/

	return moves;
}

unsigned int Position::PerftLeaf(int depth)
{
	unsigned int nodeCount = 0;

	GeneratePsudoLegalMoves();

	if (depth == 1)
	{ 
		RemoveIllegal(LegalMoves);
		return LegalMoves.size();
	}

	std::vector<Move> moves = LegalMoves;

	for (int i = 0; i < moves.size(); i++)
	{
		ApplyMove(moves[i]);

		if (IsInCheck(GetKing(!BoardParamiter.CurrentTurn), !BoardParamiter.CurrentTurn))
		{
			RevertMove(moves[i]);
			continue;					//Illegal move
		}

		uint64_t key = GenerateZobristKey();
		if ((perftTable.CheckEntry(key)) && (perftTable.GetEntry(key).GetDepth() == depth - 1))
		{
			nodeCount += perftTable.GetEntry(key).GetNodes();
			//unsigned int ChildNodeCount = PerftLeaf(depth - 1);
			//perftTable.AddEntry(key, ChildNodeCount, depth - 1);
			//nodeCount += ChildNodeCount;
		}
		else
		{
			unsigned int ChildNodeCount = PerftLeaf(depth - 1);
			perftTable.AddEntry(key, ChildNodeCount, depth - 1);
			nodeCount += ChildNodeCount;
		}
		//nodeCount += PerftLeaf(depth - 1);
		RevertMove(moves[i]);
	}

	return nodeCount;
}

unsigned int Position::AlgebraicToPos(std::string str)
{
	if (str == "-")
		return -1;
	return (static_cast<unsigned int>(str[0]) - 97) + (str[1] - 49) * 8;
}

/*ABnode * Position::SearchSetup(int depth, int cutoff)
{
	NodeCount++;
	GenerateLegalMoves();
	MoveOrder(LegalMoves);
	//return new ABnode(NONE, Move(0, 0, 0), depth, cutoff);
}*/

void Position::AddMoves(unsigned int startSq, unsigned int endSq, unsigned int flag, std::vector<Move>& Moves)
{	
	Moves.push_back(Move(startSq, endSq, flag));
}

void Position::RemoveIllegal(std::vector<Move>& moves)
{
	bool turn = BoardParamiter.CurrentTurn;
	bool Pinned[64];
	unsigned int king = GetKing(turn);

	for (int i = 0; i < 64; i++)
	{
		Pinned[i] = false;
	}

	for (int i = 0; i < moves.size(); i++)
	{
		if (moves[i].GetFlag() == EN_PASSANT || moves[i].GetFrom() == king)								//En passant and any king moves must always be checked for legality
			Pinned[moves[i].GetFrom()] = true;
	}
	
	for (int i = 0; i < 64; i++)																		//if removing the piece puts the king in check, that piece is 'pinned' and any moves from that square MUST be checked if legal
	{
		if (!Pinned[i])
		{
			if (Occupied(i, turn))
			{
				unsigned int piece = GetSquare(i);
				ClearSquare(i);
				if (IsInCheck())
					Pinned[i] = true;
				SetSquare(i, piece);
			}
		}
	}

	for (int i = 0; i < moves.size(); i++)
	{
		if (Pinned[moves[i].GetFrom()])
		{
			ApplyMove(moves[i]);

			if (IsInCheck(GetKing(turn), turn))																	//must use these arguments because a turn was applied to now boardparam.currentturn has changed
			{
				RevertMove(moves[i]);		
				moves.erase(moves.begin() + i);															//TODO possible speedup flag moves as illegal without actually having to delete them. 				
				i--;
			}
			else
				RevertMove(moves[i]);
		}
	}
}

bool Position::IsInCheck()
{
	return IsInCheck(GetKing(BoardParamiter.CurrentTurn), BoardParamiter.CurrentTurn);
}

bool Position::IsInCheck(unsigned int position, bool colour)
{
	if ((KnightAttacks[position] & GetPieces(Piece(KNIGHT, !colour))) != 0)
		return true;

	if (colour == WHITE)
	{
		if ((WhitePawnAttacks[position] & GetPieces(Piece(PAWN, !colour))) != 0)
			return true;
	}

	if (colour == BLACK)
	{
		if ((BlackPawnAttacks[position] & GetPieces(Piece(PAWN, !colour))) != 0)
			return true;
	}

	if ((KingAttacks[position] & GetPieces(Piece(KING, !colour))) != 0)					//if I can attack the enemy king he can attack me
		return true;

	uint64_t Pieces = AllPieces();

	uint64_t bishops = GetPieces(Piece(BISHOP, !colour)) & BishopAttacks[position];
	while (bishops != 0)
	{
		unsigned int start = bitScanFowardErase(bishops);
		if (mayMove(start, position, Pieces))
			return true;
	}

	uint64_t rook = GetPieces(Piece(ROOK, !colour)) & RookAttacks[position];
	while (rook != 0)
	{
		unsigned int start = bitScanFowardErase(rook);
		if (mayMove(start, position, Pieces))
			return true;
	}

	uint64_t queen = GetPieces(Piece(QUEEN, !colour)) & QueenAttacks[position];
	while (queen != 0)
	{
		unsigned int start = bitScanFowardErase(queen);
		if (mayMove(start, position, Pieces))
			return true;
	}

	return false;
}

unsigned int Position::GetKing(bool colour)
{
	uint64_t squares = GetPieces(Piece(KING, colour));

	if (squares == 0)
		return 0;
	return bitScanForward(squares);	
}

uint64_t Position::GenerateZobristKey()
{
	uint64_t Key = EMPTY;

	for (int i = 0; i < N_PIECES; i++)
	{
		uint64_t bitboard = GetPieces(i);
		while (bitboard != 0)
		{
			Key ^= ZobristTable[i * 64 + bitScanFowardErase(bitboard)];
		}
	}

	if (BoardParamiter.CurrentTurn == WHITE)
		Key ^= ZobristTable[12 * 64];

	if (BoardParamiter.WhiteKingCastle)
		Key ^= ZobristTable[12 * 64 + 1];
	if (BoardParamiter.WhiteQueenCastle)
		Key ^= ZobristTable[12 * 64 + 2];
	if (BoardParamiter.BlackKingCastle)
		Key ^= ZobristTable[12 * 64 + 3];
	if (BoardParamiter.BlackQueenCastle)
		Key ^= ZobristTable[12 * 64 + 4];

	if (BoardParamiter.EnPassant < 64)														//no ep = -1 which wraps around to a very large number
	{
		Key ^= ZobristTable[12 * 64 + 4 + GetFile(BoardParamiter.EnPassant) + 1];
	}
	

	return Key;
}

void Position::UpdateCastleRights(Move & move)
{
	if (move.GetFrom() == SQ_E1 || move.GetTo() == SQ_E1)			//Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
	{
		BoardParamiter.WhiteKingCastle = false;
		BoardParamiter.WhiteQueenCastle = false;
	}

	if (move.GetFrom() == SQ_E8 || move.GetTo() == SQ_E8)
	{
		BoardParamiter.BlackKingCastle = false;
		BoardParamiter.BlackQueenCastle = false;
	}

	if (move.GetFrom() == SQ_A1 || move.GetTo() == SQ_A1)
	{
		BoardParamiter.WhiteQueenCastle = false;
	}

	if (move.GetFrom() == SQ_A8 || move.GetTo() == SQ_A8)
	{
		BoardParamiter.BlackQueenCastle = false;
	}

	if (move.GetFrom() == SQ_H1 || move.GetTo() == SQ_H1)
	{
		BoardParamiter.WhiteKingCastle = false;
	}

	if (move.GetFrom() == SQ_H8 || move.GetTo() == SQ_H8)
	{
		BoardParamiter.BlackKingCastle = false;
	}
}

