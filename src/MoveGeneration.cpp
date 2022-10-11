#include "MoveGeneration.h"

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <vector>

#include "BoardState.h"
#include "Move.h"
#include "MoveList.h"

template <Players STM, typename T>
void GenerateLegalMoves(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t pinned); // captures and/or promotions
template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void InCheckMoves(const BoardState& board, T& moves, uint64_t pinned);

//Pawn moves
template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves); // Ep moves are always checked for legality so no need for pinned mask
template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned);

//All other pieces
template <PieceTypes pieceType, bool capture, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, uint64_t pinned);

//misc
template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves);

//utility functions
bool MovePutsSelfInCheck(const BoardState& board, const Move& move);
template <Players STM>
uint64_t PinnedMask(const BoardState& board);
bool IsSquareThreatened(const BoardState& board, Square square, Players colour); //will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
uint64_t GetThreats(const BoardState& board, Square square, Players colour); //colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!

//special generators for when in check
template <Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves); // move the king out of danger	(single or multi threat)
template <Players STM, typename T>
void KingCapturesEvade(const BoardState& board, T& moves); // use only for multi threat with king evasions
template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats); // capture the attacker	(single threat only)
template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats); // block the attacker (single threat only)

template <Players STM, typename T>
void LegalMoves(const BoardState& board, T& moves)
{
    uint64_t pinned = PinnedMask<STM>(board);

    if (IsInCheck(board))
    {
        InCheckMoves<STM>(board, moves, pinned);
    }
    else
    {
        GenerateLegalMoves<STM>(board, moves, pinned);
    }
}

template <typename T>
void LegalMoves(const BoardState& board, T& moves)
{
    switch (board.stm)
    {
    case WHITE:
        return LegalMoves<WHITE>(board, moves);
    case BLACK:
        return LegalMoves<BLACK>(board, moves);
    default:
        throw std::invalid_argument("board object without turn set");
    }
}

template <Players STM, typename T>
void InCheckMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    uint64_t Threats = GetThreats(board, board.GetKing(STM), STM);
    assert(Threats != 0);

    if (GetBitCount(Threats) > 1) //double check
    {
        KingEvasions<STM>(board, moves);
        KingCapturesEvade<STM>(board, moves);
    }
    else
    {
        PawnPushes<STM>(board, moves, pinned); //pawn moves are hard :( so we calculate those normally
        PawnDoublePushes<STM>(board, moves, pinned);
        PawnCaptures<STM>(board, moves, pinned);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned);

        KingEvasions<STM>(board, moves);
        KingCapturesEvade<STM>(board, moves);
        CaptureThreat<STM>(board, moves, Threats);
        BlockThreat<STM>(board, moves, Threats);
    }
}

template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves)
{
    switch (board.stm)
    {
    case WHITE:
        return AddQuiescenceMoves<WHITE>(board, moves, PinnedMask<WHITE>(board));
    case BLACK:
        return AddQuiescenceMoves<BLACK>(board, moves, PinnedMask<BLACK>(board));
    default:
        throw std::invalid_argument("board object without turn set");
    }
}

template <typename T>
void QuietMoves(const BoardState& board, T& moves)
{
    switch (board.stm)
    {
    case WHITE:
        return AddQuietMoves<WHITE>(board, moves, PinnedMask<WHITE>(board));
    case BLACK:
        return AddQuietMoves<BLACK>(board, moves, PinnedMask<BLACK>(board));
    default:
        throw std::invalid_argument("board object without turn set");
    }
}

template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    PawnCaptures<STM>(board, moves, pinned);
    PawnEnPassant<STM>(board, moves);
    PawnPromotions<STM>(board, moves, pinned);

    for (uint64_t pieces = board.GetPieceBB(KNIGHT, STM); pieces != 0;)
        GenerateMoves<KNIGHT, true, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(BISHOP, STM); pieces != 0;)
        GenerateMoves<BISHOP, true, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(KING, STM); pieces != 0;)
        GenerateMoves<KING, true, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(ROOK, STM); pieces != 0;)
        GenerateMoves<ROOK, true, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(QUEEN, STM); pieces != 0;)
        GenerateMoves<QUEEN, true, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
}

template <Players STM>
uint64_t PinnedMask(const BoardState& board)
{
    Square king = board.GetKing(STM);
    if (IsInCheck(board))
        return UNIVERSE;

    uint64_t mask = EMPTY;
    uint64_t possiblePins = QueenAttacks[king] & board.GetPiecesColour(STM);
    uint64_t maskAll = board.GetAllPieces();

    while (possiblePins != 0)
    {
        unsigned int sq = LSBpop(possiblePins);

        if (!mayMove(king, sq, maskAll)) //if you can't move from the square to the king, it can't be pinned
            continue;

        //If a piece is moving from the same diagonal as the king, and that diagonal contains an enemy bishop or queen
        if ((GetDiagonal(king) == GetDiagonal(sq)) && (DiagonalBB[GetDiagonal(king)] & (board.GetPieceBB(BISHOP, !STM) | board.GetPieceBB(QUEEN, !STM))))
        {
            mask |= SquareBB[sq];
            continue;
        }

        //If a piece is moving from the same anti-diagonal as the king, and that diagonal contains an enemy bishop or queen
        if ((GetAntiDiagonal(king) == GetAntiDiagonal(sq)) && (AntiDiagonalBB[GetAntiDiagonal(king)] & (board.GetPieceBB(BISHOP, !STM) | board.GetPieceBB(QUEEN, !STM))))
        {
            mask |= SquareBB[sq];
            continue;
        }

        //If a piece is moving from the same file as the king, and that file contains an enemy rook or queen
        if ((GetFile(king) == GetFile(sq)) && (FileBB[GetFile(king)] & (board.GetPieceBB(ROOK, !STM) | board.GetPieceBB(QUEEN, !STM))))
        {
            mask |= SquareBB[sq];
            continue;
        }

        //If a piece is moving from the same rank as the king, and that rank contains an enemy rook or queen
        if ((GetRank(king) == GetRank(sq)) && (RankBB[GetRank(king)] & (board.GetPieceBB(ROOK, !STM) | board.GetPieceBB(QUEEN, !STM))))
        {
            mask |= SquareBB[sq];
            continue;
        }
    }

    mask |= SquareBB[king];
    return mask;
}

//Moves going from a square to squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(Square from, uint64_t to, const BoardState& board, MoveFlag flag, T& moves, uint64_t pinned = UNIVERSE)
{
    while (to != 0)
    {
        Square target = static_cast<Square>(LSBpop(to));
        Move move(from, target, flag);
        if (!(pinned & SquareBB[from]) || !MovePutsSelfInCheck(board, move))
            moves.emplace_back(move);
    }
}

//Moves going to a square from squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(uint64_t from, Square to, const BoardState& board, MoveFlag flag, T& moves, uint64_t pinned = UNIVERSE)
{
    while (from != 0)
    {
        Square source = static_cast<Square>(LSBpop(from));
        Move move(source, to, flag);
        if (!(pinned & SquareBB[source]) || !MovePutsSelfInCheck(board, move))
            moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves)
{
    Square square = board.GetKing(STM);
    uint64_t quiets = board.GetEmptySquares() & KingAttacks[square];
    AppendLegalMoves<STM>(square, quiets, board, QUIET, moves);
}

template <Players STM, typename T>
void KingCapturesEvade(const BoardState& board, T& moves)
{
    Square square = board.GetKing(STM);
    uint64_t captures = (board.GetPiecesColour(!STM)) & KingAttacks[square];
    AppendLegalMoves<STM>(square, captures, board, CAPTURE, moves);
}

template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats)
{
    Square square = static_cast<Square>(LSBpop(threats));

    uint64_t potentialCaptures = GetThreats(board, square, !STM)
        & ~SquareBB[board.GetKing(STM)] //King captures handelled in KingCapturesEvade()
        & ~board.GetPieceBB(Piece(PAWN, STM)); //Pawn captures handelled elsewhere

    AppendLegalMoves<STM>(potentialCaptures, square, board, CAPTURE, moves);
}

template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats)
{
    Square threatSquare = static_cast<Square>(LSBpop(threats));
    Pieces piece = board.GetSquare(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; //cant block non-sliders. Also cant be threatened by enemy king

    uint64_t blockSquares = betweenArray[threatSquare][board.GetKing(STM)];

    while (blockSquares != 0)
    {
        Square square = static_cast<Square>(LSBpop(blockSquares));
        uint64_t potentialBlockers = GetThreats(board, square, !STM) & ~board.GetPieceBB(Piece(PAWN, STM)); //pawn moves need to be handelled elsewhere because they might threaten a square without being able to move there
        AppendLegalMoves<STM>(potentialBlockers, square, board, QUIET, moves);
    }
}

template <Players STM, typename T>
void GenerateLegalMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    AddQuietMoves<STM>(board, moves, pinned);
    AddQuiescenceMoves<STM>(board, moves, pinned);
}

template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    PawnPushes<STM>(board, moves, pinned);
    PawnDoublePushes<STM>(board, moves, pinned);
    CastleMoves<STM>(board, moves);

    for (uint64_t pieces = board.GetPieceBB(KNIGHT, STM); pieces != 0;)
        GenerateMoves<KNIGHT, false, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(BISHOP, STM); pieces != 0;)
        GenerateMoves<BISHOP, false, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(QUEEN, STM); pieces != 0;)
        GenerateMoves<QUEEN, false, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(ROOK, STM); pieces != 0;)
        GenerateMoves<ROOK, false, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
    for (uint64_t pieces = board.GetPieceBB(KING, STM); pieces != 0;)
        GenerateMoves<KING, false, STM>(board, moves, static_cast<Square>(LSBpop(pieces)), pinned);
}

template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned)
{
    int foward = 0;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB(PAWN, STM);

    if constexpr (STM == WHITE)
    {
        foward = 8;
        targets = (pawnSquares << 8) & board.GetEmptySquares();
    }
    if constexpr (STM == BLACK)
    {
        foward = -8;
        targets = (pawnSquares >> 8) & board.GetEmptySquares();
    }

    uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); //pushes that aren't to the back ranks

    while (pawnPushes != 0)
    {
        Square end = static_cast<Square>(LSBpop(pawnPushes));
        Square start = static_cast<Square>(end - foward);

        Move move(start, end, QUIET);

        if (!(pinned & SquareBB[start]) || !MovePutsSelfInCheck(board, move))
            moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned)
{
    int foward = 0;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB(PAWN, STM);

    if constexpr (STM == WHITE)
    {
        foward = 8;
        targets = (pawnSquares << 8) & board.GetEmptySquares();
    }
    if constexpr (STM == BLACK)
    {
        foward = -8;
        targets = (pawnSquares >> 8) & board.GetEmptySquares();
    }

    uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); //pushes that are to the back ranks

    while (pawnPromotions != 0)
    {
        Square end = static_cast<Square>(LSBpop(pawnPromotions));
        Square start = static_cast<Square>(end - foward);

        Move move(start, end, KNIGHT_PROMOTION);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck(board, move))
            continue;

        moves.emplace_back(move);
        moves.emplace_back(start, end, ROOK_PROMOTION);
        moves.emplace_back(start, end, BISHOP_PROMOTION);
        moves.emplace_back(start, end, QUEEN_PROMOTION);
    }
}

template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t pinned)
{
    int foward = 0;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB(PAWN, STM);

    if constexpr (STM == WHITE)
    {
        foward = 16;
        pawnSquares &= RankBB[RANK_2];
        targets = (pawnSquares << 8) & board.GetEmptySquares();
        targets = (targets << 8) & board.GetEmptySquares();
    }
    if constexpr (STM == BLACK)
    {
        foward = -16;
        pawnSquares &= RankBB[RANK_7];
        targets = (pawnSquares >> 8) & board.GetEmptySquares();
        targets = (targets >> 8) & board.GetEmptySquares();
    }

    while (targets != 0)
    {
        Square end = static_cast<Square>(LSBpop(targets));
        Square start = static_cast<Square>(end - foward);

        Move move(start, end, PAWN_DOUBLE_MOVE);

        if (!(pinned & SquareBB[start]) || !MovePutsSelfInCheck(board, move))
            moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        uint64_t potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.GetPieceBB(Piece(PAWN, STM));
        AppendLegalMoves<STM>(potentialAttackers, board.en_passant, board, EN_PASSANT, moves);
    }
}

template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned)
{
    int fowardleft = 0;
    int fowardright = 0;
    uint64_t leftAttack = 0;
    uint64_t rightAttack = 0;
    uint64_t pawnSquares = board.GetPieceBB(PAWN, STM);

    if constexpr (STM == WHITE)
    {
        fowardleft = 7;
        fowardright = 9;
        leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) << 7) & board.GetBlackPieces();
        rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) << 9) & board.GetBlackPieces();
    }
    if constexpr (STM == BLACK)
    {
        fowardleft = -9;
        fowardright = -7;
        leftAttack = ((pawnSquares & ~(FileBB[FILE_A])) >> 9) & board.GetWhitePieces();
        rightAttack = ((pawnSquares & ~(FileBB[FILE_H])) >> 7) & board.GetWhitePieces();
    }

    while (leftAttack != 0)
    {
        Square end = static_cast<Square>(LSBpop(leftAttack));
        Square start = static_cast<Square>(end - fowardleft);

        Move move(start, end, CAPTURE);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck(board, move))
            continue;

        if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
        {
            moves.emplace_back(start, end, KNIGHT_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, ROOK_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, BISHOP_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, QUEEN_PROMOTION_CAPTURE);
        }
        else
            moves.emplace_back(move);
    }

    while (rightAttack != 0)
    {
        Square end = static_cast<Square>(LSBpop(rightAttack));
        Square start = static_cast<Square>(end - fowardright);

        Move move(start, end, CAPTURE);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck(board, move))
            continue;

        if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
        {
            moves.emplace_back(start, end, KNIGHT_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, ROOK_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, BISHOP_PROMOTION_CAPTURE);
            moves.emplace_back(start, end, QUEEN_PROMOTION_CAPTURE);
        }
        else
            moves.emplace_back(move);
    }
}

void CastleMoves(const BoardState& board, std::vector<Move>& moves)
{
    uint64_t Pieces = board.GetAllPieces();

    if (board.white_king_castle && board.stm == WHITE)
    {
        if (mayMove(SQ_E1, SQ_H1, Pieces))
        {
            if (!IsSquareThreatened(board, SQ_E1, board.stm) && !IsSquareThreatened(board, SQ_F1, board.stm) && !IsSquareThreatened(board, SQ_G1, board.stm))
            {
                moves.emplace_back(SQ_E1, SQ_G1, KING_CASTLE);
            }
        }
    }

    if (board.white_queen_castle && board.stm == WHITE)
    {
        if (mayMove(SQ_E1, SQ_A1, Pieces))
        {
            if (!IsSquareThreatened(board, SQ_E1, board.stm) && !IsSquareThreatened(board, SQ_D1, board.stm) && !IsSquareThreatened(board, SQ_C1, board.stm))
            {
                moves.emplace_back(SQ_E1, SQ_C1, QUEEN_CASTLE);
            }
        }
    }

    if (board.black_king_castle && board.stm == BLACK)
    {
        if (mayMove(SQ_E8, SQ_H8, Pieces))
        {
            if (!IsSquareThreatened(board, SQ_E8, board.stm) && !IsSquareThreatened(board, SQ_F8, board.stm) && !IsSquareThreatened(board, SQ_G8, board.stm))
            {
                moves.emplace_back(SQ_E8, SQ_G8, KING_CASTLE);
            }
        }
    }

    if (board.black_queen_castle && board.stm == BLACK)
    {
        if (mayMove(SQ_E8, SQ_A8, Pieces))
        {
            if (!IsSquareThreatened(board, SQ_E8, board.stm) && !IsSquareThreatened(board, SQ_D8, board.stm) && !IsSquareThreatened(board, SQ_C8, board.stm))
            {
                moves.emplace_back(SQ_E8, SQ_C8, QUEEN_CASTLE);
            }
        }
    }
}

template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves)
{
    std::vector<Move> tmp;
    CastleMoves(board, tmp);
    for (auto& move : tmp)
        moves.emplace_back(move);
}

template <PieceTypes pieceType, bool capture, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, uint64_t pinned)
{
    uint64_t occupied = board.GetAllPieces();
    uint64_t targets = (capture ? board.GetPiecesColour(!STM) : ~occupied) & AttackBB<pieceType>(square, occupied);
    AppendLegalMoves<STM>(square, targets, board, capture ? CAPTURE : QUIET, moves, pinned);
}

bool IsSquareThreatened(const BoardState& board, Square square, Players colour)
{
    if ((KnightAttacks[square] & board.GetPieceBB(KNIGHT, !colour)) != 0)
        return true;

    if ((PawnAttacks[colour][square] & board.GetPieceBB(Piece(PAWN, !colour))) != 0)
        return true;

    if ((KingAttacks[square] & board.GetPieceBB(KING, !colour)) != 0) //if I can attack the enemy king he can attack me
        return true;

    uint64_t Pieces = board.GetAllPieces();

    uint64_t queen = board.GetPieceBB(QUEEN, !colour) & QueenAttacks[square];
    while (queen != 0)
    {
        unsigned int start = LSBpop(queen);
        if (mayMove(start, square, Pieces))
            return true;
    }

    uint64_t bishops = board.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
    while (bishops != 0)
    {
        unsigned int start = LSBpop(bishops);
        if (mayMove(start, square, Pieces))
            return true;
    }

    uint64_t rook = board.GetPieceBB(ROOK, !colour) & RookAttacks[square];
    while (rook != 0)
    {
        unsigned int start = LSBpop(rook);
        if (mayMove(start, square, Pieces))
            return true;
    }

    return false;
}

bool IsInCheck(const BoardState& board, Players colour)
{
    return IsSquareThreatened(board, board.GetKing(colour), colour);
}

bool IsInCheck(const BoardState& board)
{
    return IsInCheck(board, board.stm);
}

uint64_t GetThreats(const BoardState& board, Square square, Players colour)
{
    uint64_t threats = EMPTY;

    threats |= (KnightAttacks[square] & board.GetPieceBB(KNIGHT, !colour));
    threats |= (PawnAttacks[colour][square] & board.GetPieceBB(Piece(PAWN, !colour)));
    threats |= (KingAttacks[square] & board.GetPieceBB(KING, !colour));

    uint64_t Pieces = board.GetAllPieces();

    uint64_t queen = board.GetPieceBB(QUEEN, !colour) & QueenAttacks[square];
    while (queen != 0)
    {
        unsigned int start = LSBpop(queen);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    uint64_t bishops = board.GetPieceBB(BISHOP, !colour) & BishopAttacks[square];
    while (bishops != 0)
    {
        unsigned int start = LSBpop(bishops);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    uint64_t rook = board.GetPieceBB(ROOK, !colour) & RookAttacks[square];
    while (rook != 0)
    {
        unsigned int start = LSBpop(rook);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    return threats;
}

bool MoveIsLegal(const BoardState& board, const Move& move)
{
    /*Obvious check first*/
    if (move == Move::Uninitialized)
        return false;

    Pieces piece = board.GetSquare(move.GetFrom());

    /*Make sure there's a piece to be moved*/
    if (board.GetSquare(move.GetFrom()) == N_PIECES)
        return false;

    /*Make sure the piece are are moving is ours*/
    if (ColourOfPiece(piece) != board.stm)
        return false;

    /*Make sure we aren't capturing our own piece*/
    if (board.GetSquare(move.GetTo()) != N_PIECES && ColourOfPiece(board.GetSquare(move.GetTo())) == board.stm)
        return false;

    /*We don't use these flags*/
    if (move.GetFlag() == DONT_USE_1 || move.GetFlag() == DONT_USE_2)
        return false;

    uint64_t allPieces = board.GetAllPieces();

    /*Anything in the way of sliding pieces?*/
    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_ROOK || piece == BLACK_ROOK || piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if (!mayMove(move.GetFrom(), move.GetTo(), allPieces))
            return false;
    }

    /*Pawn moves are complex*/
    if (GetPieceType(piece) == PAWN)
    {
        int forward = piece == WHITE_PAWN ? 1 : -1;
        Rank startingRank = piece == WHITE_PAWN ? RANK_2 : RANK_7;

        //pawn push
        if (RankDiff(move.GetTo(), move.GetFrom()) == forward && FileDiff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (board.IsOccupied(move.GetTo())) //Something in the way!
                return false;
        }

        //pawn double push
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward * 2 && FileDiff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (GetRank(move.GetFrom()) != startingRank) //double move not from starting rank
                return false;
            if (board.IsOccupied(move.GetTo())) //something on target square
                return false;
            if (!board.IsEmpty(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2))) //something in between
                return false;
        }

        //pawn capture (not EP)
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1 && board.en_passant != move.GetTo())
        {
            if (board.IsEmpty(move.GetTo())) //nothing there to capture
                return false;
        }

        //pawn capture (EP)
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1 && board.en_passant == move.GetTo())
        {
            if (board.IsEmpty(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())))) //nothing there to capture
                return false;
        }

        else
        {
            return false;
        }
    }

    /*Check the pieces can actually move as required*/
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

    /*For castle moves, just generate them and see if we find a match*/
    if (move.GetFlag() == KING_CASTLE || move.GetFlag() == QUEEN_CASTLE)
    {
        std::vector<Move> moves;
        CastleMoves(board, moves);

        for (size_t i = 0; i < moves.size(); i++)
        {
            if (moves[i] == move)
                return true;
        }
        return false;
    }

    //-----------------------------
    //Now, we make sure the moves flag makes sense given the move

    MoveFlag flag = QUIET;

    //Captures
    if (board.IsOccupied(move.GetTo()))
        flag = CAPTURE;

    //Double pawn moves
    if (AbsRankDiff(move.GetFrom(), move.GetTo()) == 2)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    //En passant
    if (move.GetTo() == board.en_passant)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = EN_PASSANT;

    //Castling
    if (move.GetFrom() == SQ_E1 && move.GetTo() == SQ_G1 && board.GetSquare(move.GetFrom()) == WHITE_KING)
        flag = KING_CASTLE;

    if (move.GetFrom() == SQ_E1 && move.GetTo() == SQ_C1 && board.GetSquare(move.GetFrom()) == WHITE_KING)
        flag = QUEEN_CASTLE;

    if (move.GetFrom() == SQ_E8 && move.GetTo() == SQ_G8 && board.GetSquare(move.GetFrom()) == BLACK_KING)
        flag = KING_CASTLE;

    if (move.GetFrom() == SQ_E8 && move.GetTo() == SQ_C8 && board.GetSquare(move.GetFrom()) == BLACK_KING)
        flag = QUEEN_CASTLE;

    //Promotion
    if ((board.GetSquare(move.GetFrom()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_8)
        || (board.GetSquare(move.GetFrom()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_1))
    {
        if (board.IsOccupied(move.GetTo()))
        {
            if (move.GetFlag() != KNIGHT_PROMOTION_CAPTURE
                && move.GetFlag() != ROOK_PROMOTION_CAPTURE
                && move.GetFlag() != QUEEN_PROMOTION_CAPTURE
                && move.GetFlag() != BISHOP_PROMOTION_CAPTURE)
                return false;
        }
        else
        {
            if (move.GetFlag() != KNIGHT_PROMOTION
                && move.GetFlag() != ROOK_PROMOTION
                && move.GetFlag() != QUEEN_PROMOTION
                && move.GetFlag() != BISHOP_PROMOTION)
                return false;
        }
    }
    else
    {
        //check the decided on flag matches
        if (flag != move.GetFlag())
            return false;
    }
    //-----------------------------

    /*Move puts me in check*/
    if (MovePutsSelfInCheck(board, move))
        return false;

    return true;
}

/*
This function does not work for casteling moves. They are tested for legality their creation.
*/
bool MovePutsSelfInCheck(const BoardState& board, const Move& move)
{
    assert(move.GetFlag() != KING_CASTLE);
    assert(move.GetFlag() != QUEEN_CASTLE);

    BoardState copy = board;

    copy.SetSquare(move.GetTo(), copy.GetSquare(move.GetFrom()));
    copy.ClearSquare(move.GetFrom());

    if (move.GetFlag() == EN_PASSANT)
        copy.ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));

    return IsInCheck(copy, board.stm);
}

// Below code adapted with permission from Terje, author of Weiss.
//--------------------------------------------------------------------------

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
uint64_t AttackBB(Square sq, uint64_t occupied)
{
    switch (pieceType)
    {
    case KNIGHT:
        return KnightAttacks[sq];
    case KING:
        return KingAttacks[sq];
    case BISHOP:
        return bishopTable.AttackMask(sq, occupied);
    case ROOK:
        return rookTable.AttackMask(sq, occupied);
    case QUEEN:
        return AttackBB<ROOK>(sq, occupied) | AttackBB<BISHOP>(sq, occupied);
    default:
        assert(0);
    }
}

//--------------------------------------------------------------------------

// Explicit template instantiation
template void LegalMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void LegalMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void QuiescenceMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void QuiescenceMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void QuietMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void QuietMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template uint64_t AttackBB<KNIGHT>(Square sq, uint64_t occupied);
template uint64_t AttackBB<BISHOP>(Square sq, uint64_t occupied);
template uint64_t AttackBB<ROOK>(Square sq, uint64_t occupied);
template uint64_t AttackBB<QUEEN>(Square sq, uint64_t occupied);
template uint64_t AttackBB<KING>(Square sq, uint64_t occupied);
