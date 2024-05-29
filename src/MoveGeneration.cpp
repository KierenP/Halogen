#include "MoveGeneration.h"

#include <assert.h>
#include <cstddef>
#include <vector>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Magic.h"
#include "Move.h"
#include "MoveList.h"

template <Players STM, bool checks, typename T>
void LoudMoves(const BoardState& board, T& moves, uint64_t pinned);

template <Players STM, bool checks, typename T>
void LoudMovesInCheck(const BoardState& board, T& moves, uint64_t pinned, uint64_t threats);

template <Players STM, bool checks, typename T>
void QuietMoves(const BoardState& board, T& moves, uint64_t pinned);

template <Players STM, bool checks, typename T>
void QuietMovesInCheck(const BoardState& board, T& moves, uint64_t pinned, uint64_t threats);

// Pawn moves
template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t pinned);
template <Players STM, typename T>
// Ep moves are always checked for legality so no need for pinned mask
void PawnEnPassant(const BoardState& board, T& moves);
template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned);

// All other pieces
template <PieceTypes pieceType, bool capture, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, uint64_t pinned);

// misc
template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves, uint64_t pinned);

// utility functions
template <Players STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move);
template <Players STM>
uint64_t PinnedMask(const BoardState& board);
// will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square);
// colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!
template <Players colour>
uint64_t GetThreats(const BoardState& board, Square square);
template <Players STM>
bool MoveIsLegal(const BoardState& board, const Move& move);

// special generators for when in check
template <Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves); // move the king out of danger	(single or multi threat)
template <Players STM, typename T>
void KingCapturesEvade(const BoardState& board, T& moves); // use only for multi threat with king evasions
// capture the attacker	(single threat only)
template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats);
template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats); // block the attacker (single threat only)

template <Generator type, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves)
{
    auto threats = GetThreats<STM>(board, board.GetKing(STM));
    auto pinned = PinnedMask<STM>(board);

    if (threats)
    {
        if constexpr (type == MOVES_LEGAL || type == MOVES_LOUD_CHECKS)
        {
            LoudMovesInCheck<STM, true>(board, moves, pinned, threats);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_LOUD_NON_CHECKS)
        {
            LoudMovesInCheck<STM, false>(board, moves, pinned, threats);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_QUIET_CHECKS)
        {
            QuietMovesInCheck<STM, true>(board, moves, pinned, threats);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_QUIET_NON_CHECKS)
        {
            QuietMovesInCheck<STM, false>(board, moves, pinned, threats);
        }
    }
    else
    {
        if constexpr (type == MOVES_LEGAL || type == MOVES_LOUD_CHECKS)
        {
            LoudMoves<STM, true>(board, moves, pinned);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_LOUD_NON_CHECKS)
        {
            LoudMoves<STM, false>(board, moves, pinned);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_QUIET_CHECKS)
        {
            QuietMoves<STM, true>(board, moves, pinned);
        }
        if constexpr (type == MOVES_LEGAL || type == MOVES_QUIET_NON_CHECKS)
        {
            QuietMoves<STM, false>(board, moves, pinned);
        }
    }
}

template <Generator type, typename T>
void GenerateMoves(const BoardState& board, T& moves)
{
    if (board.stm == WHITE)
    {
        GenerateMoves<type, WHITE>(board, moves);
    }
    else
    {
        GenerateMoves<type, BLACK>(board, moves);
    }
}

template <Players STM, bool checks, typename T>
void QuietMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    // for simplicity, we don't consider pawn checks
    if (!checks)
    {
        PawnPushes<STM>(board, moves, pinned);
        PawnDoublePushes<STM>(board, moves, pinned);
        CastleMoves<STM>(board, moves, pinned);
    }

    for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>(); pieces != 0;)
        GenerateMoves<KNIGHT, STM, false, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
        GenerateMoves<BISHOP, STM, false, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
        GenerateMoves<QUEEN, STM, false, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
        GenerateMoves<ROOK, STM, false, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<KING, STM>(); pieces != 0;)
        GenerateMoves<KING, STM, false, checks>(board, moves, LSBpop(pieces), pinned);
}

template <Players STM, bool checks, typename T>
void QuietMovesInCheck(const BoardState& board, T& moves, uint64_t pinned, uint64_t threats)
{
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    // for now, we don't generate checking moves when in check. They are all done during the non-check generation
    if constexpr (checks)
    {
        return;
    }

    if (GetBitCount(threats) == 2)
    {
        // double check
        KingEvasions<STM>(board, moves);
    }
    else
    {
        // single check
        PawnPushes<STM>(board, moves, pinned);
        PawnDoublePushes<STM>(board, moves, pinned);
        KingEvasions<STM>(board, moves);
        BlockThreat<STM>(board, moves, threats);
    }
}

template <Players STM, bool checks, typename T>
void LoudMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    // for simplicity, we don't consider pawn checks
    if (!checks)
    {
        PawnCaptures<STM>(board, moves, pinned);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned);
    }

    for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>(); pieces != 0;)
        GenerateMoves<KNIGHT, STM, true, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
        GenerateMoves<BISHOP, STM, true, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<KING, STM>(); pieces != 0;)
        GenerateMoves<KING, STM, true, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
        GenerateMoves<ROOK, STM, true, checks>(board, moves, LSBpop(pieces), pinned);
    for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
        GenerateMoves<QUEEN, STM, true, checks>(board, moves, LSBpop(pieces), pinned);
}

template <Players STM, bool checks, typename T>
void LoudMovesInCheck(const BoardState& board, T& moves, uint64_t pinned, uint64_t threats)
{
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    // for now, we don't generate checking moves when in check. They are all done during the non-check generation
    if constexpr (checks)
    {
        return;
    }

    if (GetBitCount(threats) == 2)
    {
        // double check
        KingCapturesEvade<STM>(board, moves);
    }
    else
    {
        // single check
        PawnCaptures<STM>(board, moves, pinned);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned);
        KingCapturesEvade<STM>(board, moves);
        CaptureThreat<STM>(board, moves, threats);
    }
}

template <Players STM>
uint64_t PinnedMask(const BoardState& board)
{
    Square king = board.GetKing(STM);
    if (IsInCheck(board))
        return UNIVERSE;

    uint64_t pins = EMPTY;
    uint64_t all_pieces = board.GetAllPieces();
    uint64_t our_pieces = board.GetPieces<STM>();

    auto check_for_pins = [&](uint64_t threats)
    {
        while (threats)
        {
            Square threat_sq = LSBpop(threats);

            // get the pieces standing in between the king and the threat
            uint64_t possible_pins = betweenArray[king][threat_sq] & all_pieces;

            // if there is just one piece and it's ours it's pinned
            if (GetBitCount(possible_pins) == 1 && (possible_pins & our_pieces) != EMPTY)
            {
                pins |= SquareBB[LSBpop(possible_pins)];
            }
        }
    };

    // get the enemy bishops and queens on the kings diagonal
    const auto bishops_and_queens = board.GetPieceBB<BISHOP, !STM>() | board.GetPieceBB<QUEEN, !STM>();
    const auto rooks_and_queens = board.GetPieceBB<ROOK, !STM>() | board.GetPieceBB<QUEEN, !STM>();
    check_for_pins(DiagonalBB[GetDiagonal(king)] & bishops_and_queens);
    check_for_pins(AntiDiagonalBB[GetAntiDiagonal(king)] & bishops_and_queens);
    check_for_pins(RankBB[GetRank(king)] & rooks_and_queens);
    check_for_pins(FileBB[GetFile(king)] & rooks_and_queens);

    pins |= SquareBB[king];

    return pins;
}

// Moves going from a square to squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(
    Square from, uint64_t to, const BoardState& board, MoveFlag flag, T& moves, uint64_t pinned = UNIVERSE)
{
    while (to != 0)
    {
        Square target = LSBpop(to);
        Move move(from, target, flag);
        if (!(pinned & SquareBB[from]) || !MovePutsSelfInCheck<STM>(board, move))
            moves.emplace_back(move);
    }
}

// Moves going to a square from squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(
    uint64_t from, Square to, const BoardState& board, MoveFlag flag, T& moves, uint64_t pinned = UNIVERSE)
{
    while (from != 0)
    {
        Square source = LSBpop(from);
        Move move(source, to, flag);
        if (!(pinned & SquareBB[source]) || !MovePutsSelfInCheck<STM>(board, move))
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
    uint64_t captures = board.GetPieces<!STM>() & KingAttacks[square];
    AppendLegalMoves<STM>(square, captures, board, CAPTURE, moves);
}

template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats)
{
    Square square = LSBpop(threats);

    uint64_t potentialCaptures = GetThreats<!STM>(board, square)
        & ~SquareBB[board.GetKing(STM)] // King captures handelled in KingCapturesEvade()
        & ~board.GetPieceBB<PAWN, STM>(); // Pawn captures handelled elsewhere

    AppendLegalMoves<STM>(potentialCaptures, square, board, CAPTURE, moves);
}

template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats)
{
    Square threatSquare = LSBpop(threats);
    Pieces piece = board.GetSquare(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; // cant block non-sliders. Also cant be threatened by enemy king

    uint64_t blockSquares = betweenArray[threatSquare][board.GetKing(STM)];

    while (blockSquares != 0)
    {
        // pawn moves need to be handelled elsewhere because they might threaten a square without being able to move
        // there
        Square square = LSBpop(blockSquares);
        uint64_t potentialBlockers = GetThreats<!STM>(board, square) & ~board.GetPieceBB<PAWN, STM>();
        AppendLegalMoves<STM>(potentialBlockers, square, board, QUIET, moves);
    }
}

template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares();
    uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); // pushes that aren't to the back ranks

    while (pawnPushes != 0)
    {
        Square end = LSBpop(pawnPushes);
        Square start = end - foward;

        Move move(start, end, QUIET);

        if (!(pinned & SquareBB[start]) || !MovePutsSelfInCheck<STM>(board, move))
            moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares();
    uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); // pushes that are to the back ranks

    while (pawnPromotions != 0)
    {
        Square end = LSBpop(pawnPromotions);
        Square start = end - foward;

        Move move(start, end, KNIGHT_PROMOTION);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck<STM>(board, move))
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
    constexpr Shift foward = STM == WHITE ? Shift::NN : Shift::SS;
    uint64_t targets = 0;
    uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();

    if constexpr (STM == WHITE)
    {
        pawnSquares &= RankBB[RANK_2];
        targets = shift_bb<Shift::N>(pawnSquares) & board.GetEmptySquares();
        targets = shift_bb<Shift::N>(targets) & board.GetEmptySquares();
    }
    if constexpr (STM == BLACK)
    {
        pawnSquares &= RankBB[RANK_7];
        targets = shift_bb<Shift::S>(pawnSquares) & board.GetEmptySquares();
        targets = shift_bb<Shift::S>(targets) & board.GetEmptySquares();
    }

    while (targets != 0)
    {
        Square end = LSBpop(targets);
        Square start = end - foward;

        Move move(start, end, PAWN_DOUBLE_MOVE);

        if (!(pinned & SquareBB[start]) || !MovePutsSelfInCheck<STM>(board, move))
            moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        uint64_t potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.GetPieceBB<PAWN, STM>();
        AppendLegalMoves<STM>(potentialAttackers, board.en_passant, board, EN_PASSANT, moves);
    }
}

template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned)
{
    constexpr Shift fowardleft = STM == WHITE ? Shift::NW : Shift::SW;
    constexpr Shift fowardright = STM == WHITE ? Shift::NE : Shift::SE;
    uint64_t leftAttack = 0;
    uint64_t rightAttack = 0;
    uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    leftAttack = shift_bb<fowardleft>(pawnSquares) & board.GetPieces<!STM>();
    rightAttack = shift_bb<fowardright>(pawnSquares) & board.GetPieces<!STM>();

    while (leftAttack != 0)
    {
        Square end = LSBpop(leftAttack);
        Square start = end - fowardleft;

        Move move(start, end, CAPTURE);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck<STM>(board, move))
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
        Square end = LSBpop(rightAttack);
        Square start = end - fowardright;

        Move move(start, end, CAPTURE);
        if ((pinned & SquareBB[start]) && MovePutsSelfInCheck<STM>(board, move))
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

template <Players STM>
bool CheckCastleMove(
    const BoardState& board, Square king_start_sq, Square king_end_sq, Square rook_start_sq, Square rook_end_sq)
{
    uint64_t blockers = board.GetAllPieces() ^ SquareBB[king_start_sq] ^ SquareBB[rook_start_sq];

    if ((betweenArray[rook_start_sq][rook_end_sq] | SquareBB[rook_end_sq]) & blockers)
    {
        return false;
    }

    if ((betweenArray[king_start_sq][king_end_sq] | SquareBB[king_end_sq]) & blockers)
    {
        return false;
    }

    uint64_t king_path = betweenArray[king_start_sq][king_end_sq] | SquareBB[king_start_sq] | SquareBB[king_end_sq];

    while (king_path)
    {
        Square sq = LSBpop(king_path);
        if (IsSquareThreatened<STM>(board, sq))
        {
            return false;
        }
    }

    return true;
}

template <Players STM>
void CastleMoves(const BoardState& board, std::vector<Move>& moves, uint64_t pinned)
{
    // tricky edge case, if the rook is pinned then castling will put the king in check,
    // but it is possible none of the squares the king will move through will be threatened
    // before the rook is moved.
    uint64_t white_castle = board.castle_squares & RankBB[RANK_1] & ~pinned;

    while (STM == WHITE && white_castle)
    {
        Square king_sq = board.GetKing(WHITE);
        Square rook_sq = LSBpop(white_castle);

        if (king_sq > rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_C1, rook_sq, SQ_D1))
        {
            moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
        }
        if (king_sq < rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_G1, rook_sq, SQ_F1))
        {
            moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
        }
    }

    uint64_t black_castle = board.castle_squares & RankBB[RANK_8] & ~pinned;

    while (STM == BLACK && black_castle)
    {
        Square king_sq = board.GetKing(BLACK);
        Square rook_sq = LSBpop(black_castle);

        if (king_sq > rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_C8, rook_sq, SQ_D8))
        {
            moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
        }
        if (king_sq < rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_G8, rook_sq, SQ_F8))
        {
            moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
        }
    }
}

template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    std::vector<Move> tmp;
    CastleMoves<STM>(board, tmp, pinned);
    for (auto& move : tmp)
        moves.emplace_back(move);
}

template <PieceTypes pieceType, Players STM, bool captures, bool checkers, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, uint64_t pinned)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t check_squares = AttackBB<pieceType>(board.GetKing(!STM), occupied);
    const uint64_t targets = AttackBB<pieceType>(square, occupied) & (captures ? board.GetPieces<!STM>() : ~occupied)
        & (checkers ? check_squares : ~check_squares);

    AppendLegalMoves<STM>(square, targets, board, captures ? CAPTURE : QUIET, moves, pinned);
}

template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square)
{
    if ((KnightAttacks[square] & board.GetPieceBB<KNIGHT, !colour>()) != 0)
        return true;

    if ((PawnAttacks[colour][square] & board.GetPieceBB<PAWN, !colour>()) != 0)
        return true;

    // if I can attack the enemy king he can attack me
    if ((KingAttacks[square] & board.GetPieceBB<KING, !colour>()) != 0)
        return true;

    uint64_t Pieces = board.GetAllPieces();

    uint64_t queen = board.GetPieceBB<QUEEN, !colour>() & QueenAttacks[square];
    while (queen != 0)
    {
        auto start = LSBpop(queen);
        if (mayMove(start, square, Pieces))
            return true;
    }

    uint64_t bishops = board.GetPieceBB<BISHOP, !colour>() & BishopAttacks[square];
    while (bishops != 0)
    {
        auto start = LSBpop(bishops);
        if (mayMove(start, square, Pieces))
            return true;
    }

    uint64_t rook = board.GetPieceBB<ROOK, !colour>() & RookAttacks[square];
    while (rook != 0)
    {
        auto start = LSBpop(rook);
        if (mayMove(start, square, Pieces))
            return true;
    }

    return false;
}

template <Players colour>
bool IsInCheck(const BoardState& board)
{
    return IsSquareThreatened<colour>(board, board.GetKing(colour));
}

bool IsInCheck(const BoardState& board, Players colour)
{
    if (colour == WHITE)
    {
        return IsInCheck<WHITE>(board);
    }
    else
    {
        return IsInCheck<BLACK>(board);
    }
}

bool IsInCheck(const BoardState& board)
{
    return IsInCheck(board, board.stm);
}

template <Players colour>
uint64_t GetThreats(const BoardState& board, Square square)
{
    uint64_t threats = EMPTY;

    threats |= (KnightAttacks[square] & board.GetPieceBB<KNIGHT, !colour>());
    threats |= (PawnAttacks[colour][square] & board.GetPieceBB<PAWN, !colour>());
    threats |= (KingAttacks[square] & board.GetPieceBB<KING, !colour>());

    uint64_t Pieces = board.GetAllPieces();

    uint64_t queen = board.GetPieceBB<QUEEN, !colour>() & QueenAttacks[square];
    while (queen != 0)
    {
        auto start = LSBpop(queen);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    uint64_t bishops = board.GetPieceBB<BISHOP, !colour>() & BishopAttacks[square];
    while (bishops != 0)
    {
        auto start = LSBpop(bishops);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    uint64_t rook = board.GetPieceBB<ROOK, !colour>() & RookAttacks[square];
    while (rook != 0)
    {
        auto start = LSBpop(rook);
        if (mayMove(start, square, Pieces))
            threats |= SquareBB[start];
    }

    return threats;
}

bool MoveIsLegal(const BoardState& board, const Move& move)
{
    if (board.stm == WHITE)
    {
        return MoveIsLegal<WHITE>(board, move);
    }
    else
    {
        return MoveIsLegal<BLACK>(board, move);
    }
}

template <Players STM>
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
    if (ColourOfPiece(piece) != STM)
        return false;

    /*Make sure we aren't capturing our own piece - except when castling it's ok (chess960)*/
    if (!move.IsCastle() && board.GetSquare(move.GetTo()) != N_PIECES
        && ColourOfPiece(board.GetSquare(move.GetTo())) == STM)
        return false;

    /*We don't use these flags*/
    if (move.GetFlag() == DONT_USE_1 || move.GetFlag() == DONT_USE_2)
        return false;

    uint64_t allPieces = board.GetAllPieces();

    /*Anything in the way of sliding pieces?*/
    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_ROOK || piece == BLACK_ROOK
        || piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if (!mayMove(move.GetFrom(), move.GetTo(), allPieces))
            return false;
    }

    /*Pawn moves are complex*/
    if (GetPieceType(piece) == PAWN)
    {
        int forward = piece == WHITE_PAWN ? 1 : -1;
        Rank startingRank = piece == WHITE_PAWN ? RANK_2 : RANK_7;

        // pawn push
        if (RankDiff(move.GetTo(), move.GetFrom()) == forward && FileDiff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (board.IsOccupied(move.GetTo())) // Something in the way!
                return false;
        }

        // pawn double push
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward * 2 && FileDiff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (GetRank(move.GetFrom()) != startingRank) // double move not from starting rank
                return false;
            if (board.IsOccupied(move.GetTo())) // something on target square
                return false;
            if (!board.IsEmpty(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2))) // something in between
                return false;
        }

        // pawn capture (not EP)
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1
            && board.en_passant != move.GetTo())
        {
            if (board.IsEmpty(move.GetTo())) // nothing there to capture
                return false;
        }

        // pawn capture (EP)
        else if (RankDiff(move.GetTo(), move.GetFrom()) == forward && AbsFileDiff(move.GetFrom(), move.GetTo()) == 1
            && board.en_passant == move.GetTo())
        {
            if (board.IsEmpty(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())))) // nothing there to capture
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

    if ((piece == WHITE_KING || piece == BLACK_KING) && !move.IsCastle())
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
    if (move.GetFlag() == A_SIDE_CASTLE || move.GetFlag() == H_SIDE_CASTLE)
    {
        std::vector<Move> moves;
        CastleMoves<STM>(board, moves, PinnedMask<STM>(board));
        for (size_t i = 0; i < moves.size(); i++)
        {
            if (moves[i] == move)
                return true;
        }
        return false;
    }

    //-----------------------------
    // Now, we make sure the moves flag makes sense given the move

    MoveFlag flag = QUIET;

    // Captures
    if (board.IsOccupied(move.GetTo()))
        flag = CAPTURE;

    // Double pawn moves
    if (AbsRankDiff(move.GetFrom(), move.GetTo()) == 2)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    // En passant
    if (move.GetTo() == board.en_passant)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = EN_PASSANT;

    // Promotion
    if ((board.GetSquare(move.GetFrom()) == WHITE_PAWN && GetRank(move.GetTo()) == RANK_8)
        || (board.GetSquare(move.GetFrom()) == BLACK_PAWN && GetRank(move.GetTo()) == RANK_1))
    {
        if (board.IsOccupied(move.GetTo()))
        {
            if (move.GetFlag() != KNIGHT_PROMOTION_CAPTURE && move.GetFlag() != ROOK_PROMOTION_CAPTURE
                && move.GetFlag() != QUEEN_PROMOTION_CAPTURE && move.GetFlag() != BISHOP_PROMOTION_CAPTURE)
                return false;
        }
        else
        {
            if (move.GetFlag() != KNIGHT_PROMOTION && move.GetFlag() != ROOK_PROMOTION
                && move.GetFlag() != QUEEN_PROMOTION && move.GetFlag() != BISHOP_PROMOTION)
                return false;
        }
    }
    else
    {
        // check the decided on flag matches
        if (flag != move.GetFlag())
            return false;
    }
    //-----------------------------

    /*Move puts me in check*/
    if (MovePutsSelfInCheck<STM>(board, move))
        return false;

    return true;
}

/*
This function does not work for casteling moves. They are tested for legality their creation.
*/
template <Players STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move)
{
    assert(move.GetFlag() != A_SIDE_CASTLE);
    assert(move.GetFlag() != H_SIDE_CASTLE);

    BoardState copy = board;

    copy.SetSquare(move.GetTo(), copy.GetSquare(move.GetFrom()));
    copy.ClearSquare(move.GetFrom());

    if (move.GetFlag() == EN_PASSANT)
        copy.ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));

    return IsInCheck<STM>(copy);
}

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

// Explicit template instantiation
template void GenerateMoves<MOVES_LEGAL, BasicMoveList>(const BoardState&, BasicMoveList&);
template void GenerateMoves<MOVES_LEGAL, ExtendedMoveList>(const BoardState&, ExtendedMoveList&);

template void GenerateMoves<MOVES_LOUD_CHECKS, BasicMoveList>(const BoardState&, BasicMoveList&);
template void GenerateMoves<MOVES_LOUD_CHECKS, ExtendedMoveList>(const BoardState&, ExtendedMoveList&);

template void GenerateMoves<MOVES_LOUD_NON_CHECKS, BasicMoveList>(const BoardState&, BasicMoveList&);
template void GenerateMoves<MOVES_LOUD_NON_CHECKS, ExtendedMoveList>(const BoardState&, ExtendedMoveList&);

template void GenerateMoves<MOVES_QUIET_CHECKS, BasicMoveList>(const BoardState&, BasicMoveList&);
template void GenerateMoves<MOVES_QUIET_CHECKS, ExtendedMoveList>(const BoardState&, ExtendedMoveList&);

template void GenerateMoves<MOVES_QUIET_NON_CHECKS, BasicMoveList>(const BoardState&, BasicMoveList&);
template void GenerateMoves<MOVES_QUIET_NON_CHECKS, ExtendedMoveList>(const BoardState&, ExtendedMoveList&);

template uint64_t AttackBB<KNIGHT>(Square sq, uint64_t occupied);
template uint64_t AttackBB<BISHOP>(Square sq, uint64_t occupied);
template uint64_t AttackBB<ROOK>(Square sq, uint64_t occupied);
template uint64_t AttackBB<QUEEN>(Square sq, uint64_t occupied);
template uint64_t AttackBB<KING>(Square sq, uint64_t occupied);
