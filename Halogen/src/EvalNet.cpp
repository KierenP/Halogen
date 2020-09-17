#include "EvalNet.h"

int pieceValueVector[N_STAGES][N_PIECE_TYPES] = { {91, 532, 568, 715, 1279, 5000},
                                                  {111, 339, 372, 638, 1301, 5000} };

bool BlackBlockade(uint64_t wPawns, uint64_t bPawns);
bool WhiteBlockade(uint64_t wPawns, uint64_t bPawns);

int EvaluatePositionNet(Position& position)
{
    return std::min(4000, std::max(-4000, static_cast<int>(std::round(position.GetEvaluation()))));
}

int PieceValues(unsigned int Piece, GameStages GameStage)
{
    return pieceValueVector[GameStage][Piece % N_PIECE_TYPES];
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
