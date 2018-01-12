#pragma once
#include "BitBoard.h"
#include "Move.h"
#include "Evaluate.h"
#include "ABnode.h"
#include "TranspositionTable.h"
#include <iostream>					//for standard input and output
#include <Windows.h>				//For console colours
#include <vector>
#include <string>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

extern uint64_t PreviousPositions[1000];

struct PositionParam
{
	//PositionParam* Prev;

	bool CurrentTurn;

	bool WhiteKingCastle;
	bool WhiteQueenCastle;
	bool BlackKingCastle;
	bool BlackQueenCastle;

	unsigned int EnPassant;
	unsigned int FiftyMoveCount;
	unsigned int TurnCount;

	unsigned int CapturePiece;
	unsigned int CaptureSquare;
	unsigned int PromotionPiece;

	unsigned int GameStage = OPENING;

	bool HasCastledWhite = false;
	bool HasCastledBlack = false;
};

extern PositionParam PreviousParam[1000];

class Position : public BitBoard
{
public:
	Position();
	~Position();

	bool InitialiseFromFen(std::string board, std::string turn, std::string castle, std::string ep = "-", std::string fiftyMove = "0", std::string turnCount = "1");			//Returns true after sucsessful execution, false otherwise
	void StartingPosition();

	void Print();
	std::vector<Move>& GenerateLegalMoves();
	std::vector<Move>& GeneratePsudoLegalMoves();

	void ApplyMove(Move& move);
	void ApplyMove(std::string move);
	void RevertMove(Move& move);

	bool GetCurrentTurn();
	void SetCurrentTurn(bool turn);

	unsigned int GetGameState();
	unsigned int CalculateGameStage();

	int Evaluate();
	void EvalInit();

	double Perft(int depth);

	int BestMove(int depth, int alpha, int beta, ABnode* parent, Move prevBest, double AllowedTime);
	std::vector<Move> LegalMoves;

	int NodeCount;

	void ResetTT();
	TranspositionTable tTable;

	bool CheckThreeFold();

private:

	//Move generation functions
	void PawnPushes(std::vector<Move>& moves);
	void PawnDoublePushes(std::vector<Move>& moves);
	void PawnAttacks(std::vector<Move>& moves);
	void KnightMoves(std::vector<Move>& moves);
	void RookMoves(std::vector<Move>& moves);
	void BishopMoves(std::vector<Move>& moves);
	void QueenMoves(std::vector<Move>& moves);
	void KingMoves(std::vector<Move>& moves);
	void CastleMoves(std::vector<Move>& moves);
	void PromotionMoves(std::vector<Move>& moves);
	void RemoveIllegal(std::vector<Move>& moves);																		//remove all the moves that put the current player's king in check

	void SlidingMovesBB(std::vector<Move>& moves, uint64_t movePiece, uint64_t attackMask[N_SQUARES]);

	bool IsInCheck();
	bool IsInCheck(unsigned int position, bool colour);
	void AddMoves(unsigned int startSq, unsigned int endSq, unsigned int flag, std::vector<Move>& Moves);
	std::vector<Move>& MoveOrder(std::vector<Move>& moves, unsigned int depth);

	//Search functions
	//ABnode* SearchSetup(int depth, int cutoff);
	//int alphabetaMax(int alpha, int beta, int depth, ABnode* parent);
	//int alphabetaMin(int alpha, int beta, int depth, ABnode* parent);

	//int QuialphabetaMax(int alpha, int beta, int depth, ABnode* parent);
	//int QuialphabetaMin(int alpha, int beta, int depth, ABnode* parent);

	unsigned int PerftLeaf(int depth);

	int NegaAlphaBetaMax(ABnode* parent, int depth, int alpha, int beta, int colour, bool AllowNull);

	//Evaluation functions
	int EvalKnight(unsigned int sq, unsigned int colour);
	int EvalBishop(unsigned int sq, unsigned int colour);
	int EvalRook(unsigned int sq, unsigned int colour);
	int EvalQueen(unsigned int sq, unsigned int colour);
	int EvalKing(unsigned int sq, unsigned int colour);
	int PawnStrucureEval();
	int EvalPawn(unsigned int position, unsigned int colour);
	bool IsCheckmate();
	int KingSaftey();
	void GenerateAttackTables(uint64_t (&table)[N_PIECES]);			//white knight, white bishop, white rook, white king, etc for black. Black table for pawns and king attacks
	int CheckMateScore();

	//Paramiters
	PositionParam BoardParamiter;
	void UpdateCastleRights(Move& move);

	//
	unsigned int AlgebraicToPos(std::string str);
	unsigned int GetKing(bool colour);

	uint64_t GenerateZobristKey();
	PerftTT perftTable;
};

