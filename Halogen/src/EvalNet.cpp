#include "EvalNet.h"

int pieceValueVector[N_STAGES][N_PIECE_TYPES] = { {91, 532, 568, 715, 1279, 5000},
                                                  {111, 339, 372, 638, 1301, 5000} };

constexpr int TEMPO = 10;

void NetworkScaleAdjustment(int& eval);
void TempoAdjustment(int& eval, const Position& position);
void NoPawnAdjustment(int& eval, const Position& position);

int EvaluatePositionNet(const Position& position, EvalCacheTable& evalTable)
{
    int eval;

    if (!evalTable.GetEntry(position.GetZobristKey(), eval))
    {
        eval = position.GetEvaluation();

        NetworkScaleAdjustment(eval);
        NoPawnAdjustment(eval, position);
        TempoAdjustment(eval, position);

        evalTable.AddEntry(position.GetZobristKey(), eval);
    }

    return std::min(4000, std::max(-4000, eval));
}

void TempoAdjustment(int& eval, const Position& position)
{
    eval += position.GetTurn() == WHITE ? TEMPO : -TEMPO;
}

void NoPawnAdjustment(int& eval, const Position& position)
{
    if (eval > 0 && position.GetPieceBB(PAWN, WHITE) == 0)
        eval /= 2;
    if (eval < 0 && position.GetPieceBB(PAWN, BLACK) == 0)
        eval /= 2;
}

void NetworkScaleAdjustment(int& eval)
{
    eval = eval * 94 / 100;
}

int PieceValues(unsigned int Piece, GameStages GameStage)
{
    return pieceValueVector[GameStage][GetPieceType(static_cast<Pieces>(Piece))];
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




