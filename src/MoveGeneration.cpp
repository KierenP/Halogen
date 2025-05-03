#include "MoveGeneration.h"

#include <array>
#include <cassert>
#include <cstddef>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Magic.h"
#include "Move.h"
#include "MoveList.h" // IWYU pragma: keep
#include "StaticVector.h"

template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t checkers); // captures and/or promotions
template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t checkers);

// Pawn moves
template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t target_squares = UNIVERSE);
template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t target_squares = UNIVERSE);
template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t target_squares = UNIVERSE);
template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves);
template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t target_squares = UNIVERSE);

// All other pieces
template <bool capture, Players STM, typename T>
void GenerateKnightMoves(const BoardState& board, T& moves, Square square);
template <bool capture, Players STM, typename T>
void GenerateKingMoves(const BoardState& board, T& moves, Square from);
template <PieceTypes piece, bool capture, Players STM, typename T>
void GenerateSlidingMoves(const BoardState& board, T& moves, Square square);

// misc
template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves);

// utility functions
template <Players STM>
bool EnPassantIsLegal(const BoardState& board, const Move& move);
template <Players STM>
bool KingMoveIsLegal(const BoardState& board, const Move& move, uint64_t pinned);
template <Players STM>
uint64_t PinnedMask(const BoardState& board);
// will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square);
// colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!
template <Players colour>
uint64_t GetThreats(const BoardState& board, Square square);
template <Players STM>
bool MoveIsPsudolegal(const BoardState& board, const Move& move, uint64_t checkers);
template <Players STM>
bool MoveIsLegal(const BoardState& board, const Move& move, uint64_t pinned);

// special generators for when in check
template <bool capture, Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves, Square from, uint64_t threats);
// capture the attacker	(single threat only)
template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats);
// block the attacker (single threat only)
template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats);

template <typename T>
void LegalMoves(const BoardState& board, T& moves, uint64_t checkers)
{
    QuiescenceMoves(board, moves, checkers);
    QuietMoves(board, moves, checkers);
}

template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves, uint64_t checkers)
{
    if (board.stm == WHITE)
    {
        return AddQuiescenceMoves<WHITE>(board, moves, checkers);
    }
    else
    {
        return AddQuiescenceMoves<BLACK>(board, moves, checkers);
    }
}

template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t checkers)
{
    const Square king = board.GetKing(STM);
    assert(GetBitCount(checkers) <= 2); // triple or more check is impossible

    if (GetBitCount(checkers) == 2)
    {
        // double check
        KingEvasions<true, STM>(board, moves, king, checkers);
    }
    else if (GetBitCount(checkers) == 1)
    {
        // single check
        PawnCaptures<STM>(board, moves, checkers);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, betweenArray[LSB(checkers)][king]);
        KingEvasions<true, STM>(board, moves, king, checkers);
        CaptureThreat<STM>(board, moves, checkers);
    }
    else
    {
        // not in check
        PawnCaptures<STM>(board, moves);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves);
        GenerateKingMoves<true, STM>(board, moves, king);

        for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
            GenerateSlidingMoves<QUEEN, true, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
            GenerateSlidingMoves<ROOK, true, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
            GenerateSlidingMoves<BISHOP, true, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>(); pieces != 0;)
            GenerateKnightMoves<true, STM>(board, moves, LSBpop(pieces));
    }
}

template <typename T>
void QuietMoves(const BoardState& board, T& moves, uint64_t checkers)
{
    if (board.stm == WHITE)
    {
        return AddQuietMoves<WHITE>(board, moves, checkers);
    }
    else
    {
        return AddQuietMoves<BLACK>(board, moves, checkers);
    }
}

template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t checkers)
{
    const Square king = board.GetKing(STM);
    assert(GetBitCount(checkers) <= 2); // triple or more check is impossible

    if (GetBitCount(checkers) == 2)
    {
        // double check
        KingEvasions<false, STM>(board, moves, king, checkers);
    }
    else if (GetBitCount(checkers) == 1)
    {
        // single check
        const auto block_squares = betweenArray[LSB(checkers)][king];
        PawnPushes<STM>(board, moves, block_squares);
        PawnDoublePushes<STM>(board, moves, block_squares);
        KingEvasions<false, STM>(board, moves, king, checkers);
        BlockThreat<STM>(board, moves, checkers);
    }
    else
    {
        PawnPushes<STM>(board, moves);
        PawnDoublePushes<STM>(board, moves);
        CastleMoves<STM>(board, moves);

        for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
            GenerateSlidingMoves<QUEEN, false, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
            GenerateSlidingMoves<ROOK, false, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
            GenerateSlidingMoves<BISHOP, false, STM>(board, moves, LSBpop(pieces));
        for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>(); pieces != 0;)
            GenerateKnightMoves<false, STM>(board, moves, LSBpop(pieces));

        GenerateKingMoves<false, STM>(board, moves, king);
    }
}

uint64_t PinnedMask(const BoardState& board, Players side)
{
    if (side == WHITE)
    {
        return PinnedMask<WHITE>(board);
    }
    else
    {
        return PinnedMask<BLACK>(board);
    }
}

template <Players STM>
uint64_t PinnedMask(const BoardState& board)
{
    const Square king = board.GetKing(STM);
    const uint64_t all_pieces = board.GetAllPieces();
    const uint64_t our_pieces = board.GetPieces<STM>();
    uint64_t pins = EMPTY;

    auto check_for_pins = [&](uint64_t threats)
    {
        while (threats)
        {
            const Square threat_sq = LSBpop(threats);

            // get the pieces standing in between the king and the threat
            const uint64_t possible_pins = betweenArray[king][threat_sq] & all_pieces;

            // if there is just one piece and it's ours it's pinned
            if (GetBitCount(possible_pins) == 1 && (possible_pins & our_pieces) != EMPTY)
            {
                pins |= SquareBB[LSB(possible_pins)];
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
void AppendLegalMoves(Square from, uint64_t to, MoveFlag flag, T& moves)
{
    while (to != 0)
    {
        moves.emplace_back(from, LSBpop(to), flag);
    }
}

// Moves going to a square from squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(uint64_t from, Square to, MoveFlag flag, T& moves)
{
    while (from != 0)
    {
        moves.emplace_back(LSBpop(from), to, flag);
    }
}

template <bool capture, Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves, Square from, [[maybe_unused]] uint64_t threats)
{
    const uint64_t occupied = board.GetAllPieces();
    const auto flag = capture ? CAPTURE : QUIET;
    // const auto sliding_threats = threats
    //     & (board.GetPieceBB<QUEEN, !STM>() | board.GetPieceBB<ROOK, !STM>() | board.GetPieceBB<BISHOP, !STM>());

    uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KING>(from, occupied);

    // if there is a sliding threat, we cannot stay on that same rank/file/diagonal. But we *can* capture that threat

    /*if (sliding_threats & RankBB[GetRank(from)])
    {
        targets &= ~RankBB[GetRank(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & FileBB[GetFile(from)])
    {
        targets &= ~FileBB[GetFile(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & DiagonalBB[GetDiagonal(from)])
    {
        targets &= ~DiagonalBB[GetDiagonal(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & AntiDiagonalBB[GetAntiDiagonal(from)])
    {
        targets &= ~AntiDiagonalBB[GetAntiDiagonal(from)] | (capture ? threats : EMPTY);
    }*/

    while (targets != 0)
    {
        const Square target = LSBpop(targets);
        const Move move(from, target, flag);
        moves.emplace_back(move);
    }
}

template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats)
{
    const Square square = LSBpop(threats);

    const uint64_t potentialCaptures = GetThreats<!STM>(board, square)
        & ~SquareBB[board.GetKing(STM)] // King captures handelled in GenerateKingMoves()
        & ~board.GetPieceBB<PAWN, STM>(); // Pawn captures handelled elsewhere

    AppendLegalMoves<STM>(potentialCaptures, square, CAPTURE, moves);
}

template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats)
{
    const Square threatSquare = LSBpop(threats);
    const Pieces piece = board.GetSquare(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; // cant block non-sliders. Also cant be threatened by enemy king

    uint64_t blockSquares = betweenArray[threatSquare][board.GetKing(STM)];

    while (blockSquares != 0)
    {
        // pawn moves need to be handelled elsewhere because they might threaten a square without being able to move
        // there
        const Square square = LSBpop(blockSquares);
        const uint64_t potentialBlockers = GetThreats<!STM>(board, square) & ~board.GetPieceBB<PAWN, STM>();
        AppendLegalMoves<STM>(potentialBlockers, square, QUIET, moves);
    }
}

template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); // pushes that aren't to the back ranks

    while (pawnPushes != 0)
    {
        const Square end = LSBpop(pawnPushes);
        const Square start = end - foward;
        moves.emplace_back(start, end, QUIET);
    }
}

template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); // pushes that are to the back ranks

    while (pawnPromotions != 0)
    {
        const Square end = LSBpop(pawnPromotions);
        const Square start = end - foward;

        moves.emplace_back(start, end, KNIGHT_PROMOTION);
        moves.emplace_back(start, end, ROOK_PROMOTION);
        moves.emplace_back(start, end, BISHOP_PROMOTION);
        moves.emplace_back(start, end, QUEEN_PROMOTION);
    }
}

template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t target_squares)
{
    constexpr Shift foward2 = STM == WHITE ? Shift::NN : Shift::SS;
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    constexpr uint64_t RankMask = STM == WHITE ? RankBB[RANK_2] : RankBB[RANK_7];
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>() & RankMask;

    uint64_t targets = 0;
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares();
    targets = shift_bb<foward>(targets) & board.GetEmptySquares() & target_squares;

    while (targets != 0)
    {
        const Square end = LSBpop(targets);
        const Square start = end - foward2;
        moves.emplace_back(start, end, PAWN_DOUBLE_MOVE);
    }
}

template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        uint64_t potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.GetPieceBB<PAWN, STM>();

        while (potentialAttackers != 0)
        {
            const Square source = LSBpop(potentialAttackers);
            const Move move(source, board.en_passant, EN_PASSANT);
            moves.emplace_back(move);
        }
    }
}

template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t target_squares)
{
    constexpr Shift fowardleft = STM == WHITE ? Shift::NW : Shift::SE;
    constexpr Shift fowardright = STM == WHITE ? Shift::NE : Shift::SW;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();

    const uint64_t leftAttack = shift_bb<fowardleft>(pawnSquares) & board.GetPieces<!STM>() & target_squares;
    const uint64_t rightAttack = shift_bb<fowardright>(pawnSquares) & board.GetPieces<!STM>() & target_squares;

    auto generate_attacks = [&](uint64_t attacks, Shift forward)
    {
        while (attacks != 0)
        {
            const Square end = LSBpop(attacks);
            const Square start = end - forward;

            if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
            {
                moves.emplace_back(start, end, KNIGHT_PROMOTION_CAPTURE);
                moves.emplace_back(start, end, ROOK_PROMOTION_CAPTURE);
                moves.emplace_back(start, end, BISHOP_PROMOTION_CAPTURE);
                moves.emplace_back(start, end, QUEEN_PROMOTION_CAPTURE);
            }
            else
                moves.emplace_back(start, end, CAPTURE);
        }
    };

    generate_attacks(leftAttack, fowardleft);
    generate_attacks(rightAttack, fowardright);
}

template <Players STM>
bool CheckCastleMove(
    const BoardState& board, Square king_start_sq, Square king_end_sq, Square rook_start_sq, Square rook_end_sq)
{
    const uint64_t blockers = board.GetAllPieces() ^ SquareBB[king_start_sq] ^ SquareBB[rook_start_sq];

    if ((betweenArray[rook_start_sq][rook_end_sq] | SquareBB[rook_end_sq]) & blockers)
    {
        return false;
    }

    if ((betweenArray[king_start_sq][king_end_sq] | SquareBB[king_end_sq]) & blockers)
    {
        return false;
    }

    return true;
}

template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves)
{
    uint64_t white_castle = board.castle_squares & RankBB[RANK_1];

    while (STM == WHITE && white_castle)
    {
        const Square king_sq = board.GetKing(WHITE);
        const Square rook_sq = LSBpop(white_castle);

        if (king_sq > rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_C1, rook_sq, SQ_D1))
        {
            moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
        }
        if (king_sq < rook_sq && CheckCastleMove<STM>(board, king_sq, SQ_G1, rook_sq, SQ_F1))
        {
            moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
        }
    }

    uint64_t black_castle = board.castle_squares & RankBB[RANK_8];

    while (STM == BLACK && black_castle)
    {
        const Square king_sq = board.GetKing(BLACK);
        const Square rook_sq = LSBpop(black_castle);

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

template <bool capture, Players STM, typename T>
void GenerateKnightMoves(const BoardState& board, T& moves, Square square)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KNIGHT>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;
    AppendLegalMoves<STM>(square, targets, flag, moves);
}

template <PieceTypes piece, bool capture, Players STM, typename T>
void GenerateSlidingMoves(const BoardState& board, T& moves, Square square)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<piece>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;
    AppendLegalMoves<STM>(square, targets, flag, moves);
}

template <bool capture, Players STM, typename T>
void GenerateKingMoves(const BoardState& board, T& moves, Square square)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KING>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;
    AppendLegalMoves<STM>(square, targets, flag, moves);
}

template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square)
{
    if (AttackBB<KNIGHT>(square) & board.GetPieceBB<KNIGHT, !colour>())
        return true;
    if (PawnAttacks[colour][square] & board.GetPieceBB<PAWN, !colour>())
        return true;
    if (AttackBB<KING>(square) & board.GetPieceBB<KING, !colour>())
        return true;

    const uint64_t queens = board.GetPieceBB<QUEEN, !colour>();
    const uint64_t bishops = board.GetPieceBB<BISHOP, !colour>();
    const uint64_t rooks = board.GetPieceBB<ROOK, !colour>();
    const uint64_t occ = board.GetAllPieces();

    if (AttackBB<BISHOP>(square, occ) & (bishops | queens))
        return true;
    if (AttackBB<ROOK>(square, occ) & (rooks | queens))
        return true;

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

    const uint64_t queens = board.GetPieceBB<QUEEN, !colour>();
    const uint64_t bishops = board.GetPieceBB<BISHOP, !colour>();
    const uint64_t rooks = board.GetPieceBB<ROOK, !colour>();
    const uint64_t occ = board.GetAllPieces();

    threats |= (AttackBB<KNIGHT>(square) & board.GetPieceBB<KNIGHT, !colour>());
    threats |= (PawnAttacks[colour][square] & board.GetPieceBB<PAWN, !colour>());
    threats |= (AttackBB<KING>(square) & board.GetPieceBB<KING, !colour>());
    threats |= (AttackBB<BISHOP>(square, occ) & (bishops | queens));
    threats |= (AttackBB<ROOK>(square, occ) & (rooks | queens));

    return threats;
}

bool MoveIsPsudolegal(const BoardState& board, const Move& move, uint64_t checkers)
{
    if (board.stm == WHITE)
    {
        return MoveIsPsudolegal<WHITE>(board, move, checkers);
    }
    else
    {
        return MoveIsPsudolegal<BLACK>(board, move, checkers);
    }
}

template <Players STM>
bool MoveIsPsudolegal(const BoardState& board, const Move& move, uint64_t checkers)
{
    /*Obvious check first*/
    if (move == Move::Uninitialized)
        return false;

    const Pieces piece = board.GetSquare(move.GetFrom());

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

    const uint64_t allPieces = board.GetAllPieces();

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
        const int forward = piece == WHITE_PAWN ? 1 : -1;
        const Rank startingRank = piece == WHITE_PAWN ? RANK_2 : RANK_7;

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
    if (!checkers && (move.GetFlag() == A_SIDE_CASTLE || move.GetFlag() == H_SIDE_CASTLE))
    {
        StaticVector<Move, 4> moves;
        CastleMoves<STM>(board, moves);
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

    // MoveIsLegal makes some assumptions about moves generated when in check. We ensure those here.
    if (checkers)
    {
        // We never generate castle moves when in check
        if (move.IsCastle())
        {
            return false;
        }

        // All other regular king moves are psuedo legal.
        if (piece == WHITE_KING || piece == BLACK_KING)
        {
            return true;
        }

        if (GetBitCount(checkers) == 2)
        {
            // in double check, we only generate king moves, so if we reach this branch we know the move is not legal
            return false;
        }
        else
        {
            // in single check, we can block or capture the threat, or move the king
            if (!(SquareBB[move.GetTo()] & (checkers | betweenArray[LSB(checkers)][board.GetKing(STM)])))
            {
                return false;
            }
        }
    }

    return true;
}

bool MoveIsLegal(const BoardState& board, const Move& move, uint64_t pinned)
{
    if (board.stm == WHITE)
    {
        return MoveIsLegal<WHITE>(board, move, pinned);
    }
    else
    {
        return MoveIsLegal<BLACK>(board, move, pinned);
    }
}

template <Players STM>
bool MoveIsLegal(const BoardState& board, const Move& move, uint64_t pinned)
{
    if (move.IsCastle())
    {
        const Square king_start = move.GetFrom();
        const Square king_end
            = GetPosition(move.GetFlag() == A_SIDE_CASTLE ? FILE_C : FILE_G, STM == WHITE ? RANK_1 : RANK_8);
        uint64_t king_path = betweenArray[king_start][king_end] | SquareBB[king_start] | SquareBB[king_end];

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

    if (move.GetFlag() == EN_PASSANT)
    {
        return EnPassantIsLegal<STM>(board, move);
    }

    const auto piece = board.GetSquare(move.GetFrom());

    if (GetPieceType(piece) == KING)
    {
        return KingMoveIsLegal<STM>(board, move);
    }

    if (pinned & SquareBB[move.GetFrom()])
    {
        // A pinned piece must stay on the king ray
        return RayBB[board.GetKing(STM)][move.GetFrom()] & SquareBB[move.GetTo()];
    }
    else
    {
        return true;
    }
}

uint64_t Checkers(const BoardState& board)
{
    if (board.stm == WHITE)
    {
        return GetThreats<WHITE>(board, board.GetKing(WHITE));
    }
    else
    {
        return GetThreats<BLACK>(board, board.GetKing(BLACK));
    }
}

bool EnPassantIsLegal(const BoardState& board, const Move& move)
{
    if (board.stm == WHITE)
    {
        return EnPassantIsLegal<WHITE>(board, move);
    }
    else
    {
        return EnPassantIsLegal<BLACK>(board, move);
    }
}

template <Players STM>
bool EnPassantIsLegal(const BoardState& board, const Move& move)
{
    const Square king = board.GetKing(STM);

    if (AttackBB<KNIGHT>(king) & board.GetPieceBB<KNIGHT, !STM>())
        return false;

    if (AttackBB<KING>(king) & board.GetPieceBB<KING, !STM>())
        return false;

    const uint64_t pawns
        = board.GetPieceBB<PAWN, !STM>() & ~SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];
    if (PawnAttacks[STM][king] & pawns)
        return false;

    const uint64_t queens = board.GetPieceBB<QUEEN, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t bishops = board.GetPieceBB<BISHOP, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t rooks = board.GetPieceBB<ROOK, !STM>() & ~SquareBB[move.GetTo()];
    uint64_t occ = board.GetAllPieces();

    occ &= ~SquareBB[move.GetFrom()];
    occ &= ~SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];
    occ |= SquareBB[move.GetTo()];

    if (AttackBB<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (AttackBB<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <Players STM>
bool KingMoveIsLegal(const BoardState& board, const Move& move)
{
    const Square king = move.GetTo();

    if (AttackBB<KNIGHT>(king) & board.GetPieceBB<KNIGHT, !STM>())
        return false;

    if (PawnAttacks[STM][king] & board.GetPieceBB<PAWN, !STM>())
        return false;

    if (AttackBB<KING>(king) & board.GetPieceBB<KING, !STM>())
        return false;

    const uint64_t queens = board.GetPieceBB<QUEEN, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t bishops = board.GetPieceBB<BISHOP, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t rooks = board.GetPieceBB<ROOK, !STM>() & ~SquareBB[move.GetTo()];
    uint64_t occ = board.GetAllPieces();

    occ &= ~SquareBB[move.GetFrom()];
    occ |= SquareBB[move.GetTo()];

    if (AttackBB<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (AttackBB<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <Players us>
uint64_t ThreatMask(const BoardState& board)
{
    const uint64_t occ = board.GetAllPieces();

    uint64_t threats = EMPTY;
    uint64_t vulnerable = board.GetPieces<!us>();

    // Pawn capture non-pawn
    vulnerable ^= board.GetPieceBB<PAWN, !us>();
    for (uint64_t pieces = board.GetPieceBB<PAWN, us>(); pieces != 0;)
    {
        threats |= PawnAttacks[us][LSBpop(pieces)] & vulnerable;
    }

    // Bishop/Knight capture Rook/Queen
    vulnerable ^= board.GetPieceBB<KNIGHT, !us>() | board.GetPieceBB<BISHOP, !us>();
    for (uint64_t pieces = board.GetPieceBB<KNIGHT, us>(); pieces != 0;)
    {
        threats |= AttackBB<KNIGHT>(LSBpop(pieces), occ) & vulnerable;
    }
    for (uint64_t pieces = board.GetPieceBB<BISHOP, us>(); pieces != 0;)
    {
        threats |= AttackBB<BISHOP>(LSBpop(pieces), occ) & vulnerable;
    }

    // Rook capture queen
    vulnerable ^= board.GetPieceBB<ROOK, !us>();
    for (uint64_t pieces = board.GetPieceBB<ROOK, us>(); pieces != 0;)
    {
        threats |= AttackBB<ROOK>(LSBpop(pieces), occ) & vulnerable;
    }

    return threats;
}

uint64_t ThreatMask(const BoardState& board, Players colour)
{
    if (colour == WHITE)
    {
        return ThreatMask<WHITE>(board);
    }
    else
    {
        return ThreatMask<BLACK>(board);
    }
}

template <>
uint64_t AttackBB<KNIGHT>(Square sq, uint64_t)
{
    return KnightAttacks[sq];
}

template <>
uint64_t AttackBB<BISHOP>(Square sq, uint64_t occupied)
{
    return bishopTable.AttackMask(sq, occupied);
}

template <>
uint64_t AttackBB<ROOK>(Square sq, uint64_t occupied)
{
    return rookTable.AttackMask(sq, occupied);
}

template <>
uint64_t AttackBB<QUEEN>(Square sq, uint64_t occupied)
{
    return AttackBB<ROOK>(sq, occupied) | AttackBB<BISHOP>(sq, occupied);
}

template <>
uint64_t AttackBB<KING>(Square sq, uint64_t)
{
    return KingAttacks[sq];
}

// Explicit template instantiation
template void LegalMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves, uint64_t checkers);
template void LegalMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves, uint64_t checkers);

template void QuiescenceMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves, uint64_t checkers);
template void QuiescenceMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves, uint64_t checkers);

template void QuietMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves, uint64_t checkers);
template void QuietMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves, uint64_t checkers);
