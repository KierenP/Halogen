#include "MoveGeneration.h"

#include <array>
#include <cassert>
#include <cstddef>

#include "BoardState.h"
#include "Magic.h"
#include "Move.h"
#include "MoveList.h" // IWYU pragma: keep
#include "StaticVector.h"
#include "bitboard.h"

template <Side STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t pinned); // captures and/or promotions
template <Side STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t pinned);

// Pawn moves
template <Side STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
// Ep moves are always checked for legality so no need for pinned mask
void PawnEnPassant(const BoardState& board, T& moves);
template <Side STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);

// All other pieces
template <bool capture, Side STM, typename T>
void GenerateKnightMoves(const BoardState& board, T& moves, Square square);
template <bool capture, Side STM, typename T>
void GenerateKingMoves(const BoardState& board, T& moves, Square from);
template <PieceType piece, bool capture, Side STM, typename T>
void GenerateSlidingMoves(const BoardState& board, T& moves, Square square, uint64_t pinned, Square king);

// misc
template <Side STM, typename T>
void CastleMoves(const BoardState& board, T& moves, uint64_t pinned);

// utility functions
template <Side STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move);
template <Side STM>
bool EnPassantIsLegal(const BoardState& board, const Move& move);
template <Side STM>
bool KingMoveIsLegal(const BoardState& board, const Move& move);
template <Side STM>
uint64_t PinnedMask(const BoardState& board);
// will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
template <Side colour>
bool IsSquareThreatened(const BoardState& board, Square square);
// colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!
template <Side colour>
uint64_t GetThreats(const BoardState& board, Square square);
template <Side STM>
bool MoveIsLegal(const BoardState& board, const Move& move);

// special generators for when in check
template <bool capture, Side STM, typename T>
void KingEvasions(const BoardState& board, T& moves, Square from, uint64_t threats);
// capture the attacker	(single threat only)
template <Side STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned);
// block the attacker (single threat only)
template <Side STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned);

template <typename T>
void LegalMoves(const BoardState& board, T& moves)
{
    QuiescenceMoves(board, moves);
    QuietMoves(board, moves);
}

template <typename T>
void QuiescenceMoves(const BoardState& board, T& moves)
{
    if (board.stm == WHITE)
    {
        return AddQuiescenceMoves<WHITE>(board, moves, PinnedMask<WHITE>(board));
    }
    else
    {
        return AddQuiescenceMoves<BLACK>(board, moves, PinnedMask<BLACK>(board));
    }
}

template <Side STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    const Square king = board.GetKing(STM);
    const uint64_t threats = GetThreats<STM>(board, king);
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    if (popcount(threats) == 2)
    {
        // double check
        KingEvasions<true, STM>(board, moves, king, threats);
    }
    else if (popcount(threats) == 1)
    {
        // single check
        PawnCaptures<STM>(board, moves, pinned, threats);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned, BetweenBB[lsb(threats)][king]);
        KingEvasions<true, STM>(board, moves, king, threats);
        CaptureThreat<STM>(board, moves, threats, pinned);
    }
    else
    {
        // not in check
        PawnCaptures<STM>(board, moves, pinned);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned);
        GenerateKingMoves<true, STM>(board, moves, king);

        for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
            GenerateSlidingMoves<QUEEN, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
            GenerateSlidingMoves<ROOK, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
            GenerateSlidingMoves<BISHOP, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>() & ~pinned; pieces != 0;)
            GenerateKnightMoves<true, STM>(board, moves, lsbpop(pieces));
    }
}

template <typename T>
void QuietMoves(const BoardState& board, T& moves)
{
    if (board.stm == WHITE)
    {
        return AddQuietMoves<WHITE>(board, moves, PinnedMask<WHITE>(board));
    }
    else
    {
        return AddQuietMoves<BLACK>(board, moves, PinnedMask<BLACK>(board));
    }
}

template <Side STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    const Square king = board.GetKing(STM);
    const uint64_t threats = GetThreats<STM>(board, king);
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    if (popcount(threats) == 2)
    {
        // double check
        KingEvasions<false, STM>(board, moves, king, threats);
    }
    else if (popcount(threats) == 1)
    {
        // single check
        const auto block_squares = BetweenBB[lsb(threats)][king];
        PawnPushes<STM>(board, moves, pinned, block_squares);
        PawnDoublePushes<STM>(board, moves, pinned, block_squares);
        KingEvasions<false, STM>(board, moves, king, threats);
        BlockThreat<STM>(board, moves, threats, pinned);
    }
    else
    {
        PawnPushes<STM>(board, moves, pinned);
        PawnDoublePushes<STM>(board, moves, pinned);
        CastleMoves<STM>(board, moves, pinned);

        for (uint64_t pieces = board.GetPieceBB<QUEEN, STM>(); pieces != 0;)
            GenerateSlidingMoves<QUEEN, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<ROOK, STM>(); pieces != 0;)
            GenerateSlidingMoves<ROOK, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<BISHOP, STM>(); pieces != 0;)
            GenerateSlidingMoves<BISHOP, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.GetPieceBB<KNIGHT, STM>() & ~pinned; pieces != 0;)
            GenerateKnightMoves<false, STM>(board, moves, lsbpop(pieces));

        GenerateKingMoves<false, STM>(board, moves, king);
    }
}

template <Side STM>
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
            const Square threat_sq = lsbpop(threats);

            // get the pieces standing in between the king and the threat
            const uint64_t possible_pins = BetweenBB[king][threat_sq] & all_pieces;

            // if there is just one piece and it's ours it's pinned
            if (popcount(possible_pins) == 1 && (possible_pins & our_pieces) != EMPTY)
            {
                pins |= SquareBB[lsb(possible_pins)];
            }
        }
    };

    // get the enemy bishops and queens on the kings diagonal
    const auto bishops_and_queens = board.GetPieceBB<BISHOP, !STM>() | board.GetPieceBB<QUEEN, !STM>();
    const auto rooks_and_queens = board.GetPieceBB<ROOK, !STM>() | board.GetPieceBB<QUEEN, !STM>();
    check_for_pins(DiagonalBB[enum_to<Diagonal>(king)] & bishops_and_queens);
    check_for_pins(AntiDiagonalBB[enum_to<AntiDiagonal>(king)] & bishops_and_queens);
    check_for_pins(RankBB[enum_to<Rank>(king)] & rooks_and_queens);
    check_for_pins(FileBB[enum_to<File>(king)] & rooks_and_queens);

    pins |= SquareBB[king];

    return pins;
}

// Moves going from a square to squares on a bitboard
template <Side STM, typename T>
void AppendLegalMoves(Square from, uint64_t to, MoveFlag flag, T& moves)
{
    while (to != 0)
    {
        moves.emplace_back(from, lsbpop(to), flag);
    }
}

// Moves going to a square from squares on a bitboard
template <Side STM, typename T>
void AppendLegalMoves(uint64_t from, Square to, MoveFlag flag, T& moves)
{
    while (from != 0)
    {
        moves.emplace_back(lsbpop(from), to, flag);
    }
}

template <bool capture, Side STM, typename T>
void KingEvasions(const BoardState& board, T& moves, Square from, uint64_t threats)
{
    const uint64_t occupied = board.GetAllPieces();
    const auto flag = capture ? CAPTURE : QUIET;
    const auto sliding_threats = threats
        & (board.GetPieceBB<QUEEN, !STM>() | board.GetPieceBB<ROOK, !STM>() | board.GetPieceBB<BISHOP, !STM>());

    uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KING>(from, occupied);

    // if there is a sliding threat, we cannot stay on that same rank/file/diagonal. But we *can* capture that threat

    if (sliding_threats & RankBB[enum_to<Rank>(from)])
    {
        targets &= ~RankBB[enum_to<Rank>(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & FileBB[enum_to<File>(from)])
    {
        targets &= ~FileBB[enum_to<File>(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & DiagonalBB[enum_to<Diagonal>(from)])
    {
        targets &= ~DiagonalBB[enum_to<Diagonal>(from)] | (capture ? threats : EMPTY);
    }

    if (sliding_threats & AntiDiagonalBB[enum_to<AntiDiagonal>(from)])
    {
        targets &= ~AntiDiagonalBB[enum_to<AntiDiagonal>(from)] | (capture ? threats : EMPTY);
    }

    while (targets != 0)
    {
        const Square target = lsbpop(targets);
        const Move move(from, target, flag);
        if (KingMoveIsLegal<STM>(board, move))
        {
            moves.emplace_back(move);
        }
    }
}

template <Side STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned)
{
    const Square square = lsbpop(threats);

    const uint64_t potentialCaptures = GetThreats<!STM>(board, square)
        & ~SquareBB[board.GetKing(STM)] // King captures handelled in GenerateKingMoves()
        & ~board.GetPieceBB<PAWN, STM>() // Pawn captures handelled elsewhere
        & ~pinned; // any pinned pieces cannot legally capture the threat

    AppendLegalMoves<STM>(potentialCaptures, square, CAPTURE, moves);
}

template <Side STM, typename T>
void BlockThreat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned)
{
    const Square threatSquare = lsbpop(threats);
    const Piece piece = board.GetSquare(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; // cant block non-sliders. Also cant be threatened by enemy king

    uint64_t blockSquares = BetweenBB[threatSquare][board.GetKing(STM)];

    while (blockSquares != 0)
    {
        // pawn moves need to be handelled elsewhere because they might threaten a square without being able to move
        // there
        const Square square = lsbpop(blockSquares);
        // blocking moves are legal iff the piece is not pinned
        const uint64_t potentialBlockers = GetThreats<!STM>(board, square) & ~board.GetPieceBB<PAWN, STM>() & ~pinned;
        AppendLegalMoves<STM>(potentialBlockers, square, QUIET, moves);
    }
}

template <Side STM, typename T>
void PawnPushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); // pushes that aren't to the back ranks

    while (pawnPushes != 0)
    {
        const Square end = lsbpop(pawnPushes);
        const Square start = end - foward;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if (!(pinned & SquareBB[start]) || (enum_to<File>(start) == enum_to<File>(board.GetKing(STM))))
        {
            moves.emplace_back(start, end, QUIET);
        }
    }
}

template <Side STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); // pushes that are to the back ranks

    while (pawnPromotions != 0)
    {
        const Square end = lsbpop(pawnPromotions);
        const Square start = end - foward;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if ((pinned & SquareBB[start]) && (enum_to<File>(start) != enum_to<File>(board.GetKing(STM))))
            continue;

        moves.emplace_back(start, end, KNIGHT_PROMOTION);
        moves.emplace_back(start, end, ROOK_PROMOTION);
        moves.emplace_back(start, end, BISHOP_PROMOTION);
        moves.emplace_back(start, end, QUEEN_PROMOTION);
    }
}

template <Side STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
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
        const Square end = lsbpop(targets);
        const Square start = end - foward2;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if (!(pinned & SquareBB[start]) || (enum_to<File>(start) == enum_to<File>(board.GetKing(STM))))
            moves.emplace_back(start, end, PAWN_DOUBLE_MOVE);
    }
}

template <Side STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        uint64_t potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.GetPieceBB<PAWN, STM>();

        while (potentialAttackers != 0)
        {
            const Square source = lsbpop(potentialAttackers);
            const Move move(source, board.en_passant, EN_PASSANT);
            if (EnPassantIsLegal<STM>(board, move))
                moves.emplace_back(move);
        }
    }
}

template <Side STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift fowardleft = STM == WHITE ? Shift::NW : Shift::SE;
    constexpr Shift fowardright = STM == WHITE ? Shift::NE : Shift::SW;
    const uint64_t pawnSquares = board.GetPieceBB<PAWN, STM>();

    const uint64_t leftAttack = shift_bb<fowardleft>(pawnSquares) & board.GetPieces<!STM>() & target_squares;
    const uint64_t rightAttack = shift_bb<fowardright>(pawnSquares) & board.GetPieces<!STM>() & target_squares;

    auto generate_attacks = [&](uint64_t attacks, Shift forward, auto get_cardinal)
    {
        while (attacks != 0)
        {
            const Square end = lsbpop(attacks);
            const Square start = end - forward;

            // If we are pinned, the move is legal iff we are on the same [anti]diagonal as the king
            if ((pinned & SquareBB[start]) && (get_cardinal(start) != get_cardinal(board.GetKing(STM))))
                continue;

            if (enum_to<Rank>(end) == RANK_1 || enum_to<Rank>(end) == RANK_8)
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

    generate_attacks(leftAttack, fowardleft, enum_to<AntiDiagonal, Square>);
    generate_attacks(rightAttack, fowardright, enum_to<Diagonal, Square>);
}

template <Side STM>
bool CheckCastleMove(
    const BoardState& board, Square king_start_sq, Square king_end_sq, Square rook_start_sq, Square rook_end_sq)
{
    const uint64_t blockers = board.GetAllPieces() ^ SquareBB[king_start_sq] ^ SquareBB[rook_start_sq];

    if ((BetweenBB[rook_start_sq][rook_end_sq] | SquareBB[rook_end_sq]) & blockers)
    {
        return false;
    }

    if ((BetweenBB[king_start_sq][king_end_sq] | SquareBB[king_end_sq]) & blockers)
    {
        return false;
    }

    uint64_t king_path = BetweenBB[king_start_sq][king_end_sq] | SquareBB[king_start_sq] | SquareBB[king_end_sq];

    while (king_path)
    {
        Square sq = lsbpop(king_path);
        if (IsSquareThreatened<STM>(board, sq))
        {
            return false;
        }
    }

    return true;
}

template <Side STM, typename T>
void CastleMoves(const BoardState& board, T& moves, uint64_t pinned)
{
    // tricky edge case, if the rook is pinned then castling will put the king in check,
    // but it is possible none of the squares the king will move through will be threatened
    // before the rook is moved.
    uint64_t white_castle = board.castle_squares & RankBB[RANK_1] & ~pinned;

    while (STM == WHITE && white_castle)
    {
        const Square king_sq = board.GetKing(WHITE);
        const Square rook_sq = lsbpop(white_castle);

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
        const Square king_sq = board.GetKing(BLACK);
        const Square rook_sq = lsbpop(black_castle);

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

template <bool capture, Side STM, typename T>
void GenerateKnightMoves(const BoardState& board, T& moves, Square square)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KNIGHT>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;
    AppendLegalMoves<STM>(square, targets, flag, moves);
}

template <PieceType piece, bool capture, Side STM, typename T>
void GenerateSlidingMoves(const BoardState& board, T& moves, Square square, uint64_t pinned, Square king)
{
    const uint64_t occupied = board.GetAllPieces();
    const uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<piece>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;

    if (!(pinned & SquareBB[square]))
    {
        AppendLegalMoves<STM>(square, targets, flag, moves);
    }
    else
    {
        AppendLegalMoves<STM>(square, targets & RayBB[king][square], flag, moves);
    }
}

template <bool capture, Side STM, typename T>
void GenerateKingMoves(const BoardState& board, T& moves, Square from)
{
    const uint64_t occupied = board.GetAllPieces();
    const auto flag = capture ? CAPTURE : QUIET;

    uint64_t targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<KING>(from, occupied);

    while (targets != 0)
    {
        const Square target = lsbpop(targets);
        const Move move(from, target, flag);
        if (KingMoveIsLegal<STM>(board, move))
        {
            moves.emplace_back(move);
        }
    }
}

template <Side colour>
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

template <Side colour>
bool IsInCheck(const BoardState& board)
{
    return IsSquareThreatened<colour>(board, board.GetKing(colour));
}

bool IsInCheck(const BoardState& board, Side colour)
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

template <Side colour>
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

template <Side STM>
bool MoveIsLegal(const BoardState& board, const Move& move)
{
    /*Obvious check first*/
    if (move == Move::Uninitialized)
        return false;

    const Piece piece = board.GetSquare(move.GetFrom());

    /*Make sure there's a piece to be moved*/
    if (board.GetSquare(move.GetFrom()) == N_PIECES)
        return false;

    /*Make sure the piece are are moving is ours*/
    if (enum_to<Side>(piece) != STM)
        return false;

    /*Make sure we aren't capturing our own piece - except when castling it's ok (chess960)*/
    if (!move.IsCastle() && board.GetSquare(move.GetTo()) != N_PIECES
        && enum_to<Side>(board.GetSquare(move.GetTo())) == STM)
        return false;

    /*We don't use these flags*/
    if (move.GetFlag() == DONT_USE_1 || move.GetFlag() == DONT_USE_2)
        return false;

    const uint64_t allPieces = board.GetAllPieces();

    /*Anything in the way of sliding pieces?*/
    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_ROOK || piece == BLACK_ROOK
        || piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if (!path_clear(move.GetFrom(), move.GetTo(), allPieces))
            return false;
    }

    /*Pawn moves are complex*/
    if (enum_to<PieceType>(piece) == PAWN)
    {
        const int forward = piece == WHITE_PAWN ? 1 : -1;
        const Rank startingRank = piece == WHITE_PAWN ? RANK_2 : RANK_7;

        // pawn push
        if (rank_diff(move.GetTo(), move.GetFrom()) == forward && file_diff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (board.IsOccupied(move.GetTo())) // Something in the way!
                return false;
        }

        // pawn double push
        else if (rank_diff(move.GetTo(), move.GetFrom()) == forward * 2 && file_diff(move.GetFrom(), move.GetTo()) == 0)
        {
            if (enum_to<Rank>(move.GetFrom()) != startingRank) // double move not from starting rank
                return false;
            if (board.IsOccupied(move.GetTo())) // something on target square
                return false;
            if (!board.IsEmpty(static_cast<Square>((move.GetTo() + move.GetFrom()) / 2))) // something in between
                return false;
        }

        // pawn capture (not EP)
        else if (rank_diff(move.GetTo(), move.GetFrom()) == forward && abs_file_diff(move.GetFrom(), move.GetTo()) == 1
            && board.en_passant != move.GetTo())
        {
            if (board.IsEmpty(move.GetTo())) // nothing there to capture
                return false;
        }

        // pawn capture (EP)
        else if (rank_diff(move.GetTo(), move.GetFrom()) == forward && abs_file_diff(move.GetFrom(), move.GetTo()) == 1
            && board.en_passant == move.GetTo())
        {
            if (board.IsEmpty(
                    get_square(enum_to<File>(move.GetTo()), enum_to<Rank>(move.GetFrom())))) // nothing there to capture
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
        StaticVector<Move, 4> moves;
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
    if (abs_rank_diff(move.GetFrom(), move.GetTo()) == 2)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    // En passant
    if (move.GetTo() == board.en_passant)
        if (board.GetSquare(move.GetFrom()) == WHITE_PAWN || board.GetSquare(move.GetFrom()) == BLACK_PAWN)
            flag = EN_PASSANT;

    // Promotion
    if ((board.GetSquare(move.GetFrom()) == WHITE_PAWN && enum_to<Rank>(move.GetTo()) == RANK_8)
        || (board.GetSquare(move.GetFrom()) == BLACK_PAWN && enum_to<Rank>(move.GetTo()) == RANK_1))
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
template <Side STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move)
{
    assert(move.GetFlag() != A_SIDE_CASTLE);
    assert(move.GetFlag() != H_SIDE_CASTLE);

    const Square king = enum_to<PieceType>(board.GetSquare(move.GetFrom())) == KING ? move.GetTo() : board.GetKing(STM);

    const uint64_t knights = board.GetPieceBB<KNIGHT, !STM>() & ~SquareBB[move.GetTo()];
    if (AttackBB<KNIGHT>(king) & knights)
        return true;

    uint64_t pawns = board.GetPieceBB<PAWN, !STM>() & ~SquareBB[move.GetTo()];

    if (move.GetFlag() == EN_PASSANT)
        pawns &= ~SquareBB[get_square(enum_to<File>(move.GetTo()), enum_to<Rank>(move.GetFrom()))];

    if (PawnAttacks[STM][king] & pawns)
        return true;

    if (AttackBB<KING>(king) & board.GetPieceBB<KING, !STM>())
        return true;

    const uint64_t queens = board.GetPieceBB<QUEEN, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t bishops = board.GetPieceBB<BISHOP, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t rooks = board.GetPieceBB<ROOK, !STM>() & ~SquareBB[move.GetTo()];
    uint64_t occ = board.GetAllPieces();

    occ &= ~SquareBB[move.GetFrom()];
    occ |= SquareBB[move.GetTo()];

    if (move.GetFlag() == EN_PASSANT)
        occ &= ~SquareBB[get_square(enum_to<File>(move.GetTo()), enum_to<Rank>(move.GetFrom()))];

    if (AttackBB<BISHOP>(king, occ) & (bishops | queens))
        return true;
    if (AttackBB<ROOK>(king, occ) & (rooks | queens))
        return true;

    return false;
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

template <Side STM>
bool EnPassantIsLegal(const BoardState& board, const Move& move)
{
    const Square king = board.GetKing(STM);

    if (AttackBB<KNIGHT>(king) & board.GetPieceBB<KNIGHT, !STM>())
        return false;

    if (AttackBB<KING>(king) & board.GetPieceBB<KING, !STM>())
        return false;

    const uint64_t pawns = board.GetPieceBB<PAWN, !STM>()
        & ~SquareBB[get_square(enum_to<File>(move.GetTo()), enum_to<Rank>(move.GetFrom()))];
    if (PawnAttacks[STM][king] & pawns)
        return false;

    const uint64_t queens = board.GetPieceBB<QUEEN, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t bishops = board.GetPieceBB<BISHOP, !STM>() & ~SquareBB[move.GetTo()];
    const uint64_t rooks = board.GetPieceBB<ROOK, !STM>() & ~SquareBB[move.GetTo()];
    uint64_t occ = board.GetAllPieces();

    occ &= ~SquareBB[move.GetFrom()];
    occ &= ~SquareBB[get_square(enum_to<File>(move.GetTo()), enum_to<Rank>(move.GetFrom()))];
    occ |= SquareBB[move.GetTo()];

    if (AttackBB<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (AttackBB<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <Side STM>
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

    // where this function is called, we already filter out attacks going through the 'from' square, so we don't need to
    // update occ to apply the king move

    if (AttackBB<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (AttackBB<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <Side us>
uint64_t ThreatMask(const BoardState& board)
{
    const uint64_t occ = board.GetAllPieces();

    uint64_t threats = EMPTY;
    uint64_t vulnerable = board.GetPieces<!us>();

    // Pawn capture non-pawn
    vulnerable ^= board.GetPieceBB<PAWN, !us>();
    for (uint64_t pieces = board.GetPieceBB<PAWN, us>(); pieces != 0;)
    {
        threats |= PawnAttacks[us][lsbpop(pieces)] & vulnerable;
    }

    // Bishop/Knight capture Rook/Queen
    vulnerable ^= board.GetPieceBB<KNIGHT, !us>() | board.GetPieceBB<BISHOP, !us>();
    for (uint64_t pieces = board.GetPieceBB<KNIGHT, us>(); pieces != 0;)
    {
        threats |= AttackBB<KNIGHT>(lsbpop(pieces), occ) & vulnerable;
    }
    for (uint64_t pieces = board.GetPieceBB<BISHOP, us>(); pieces != 0;)
    {
        threats |= AttackBB<BISHOP>(lsbpop(pieces), occ) & vulnerable;
    }

    // Rook capture queen
    vulnerable ^= board.GetPieceBB<ROOK, !us>();
    for (uint64_t pieces = board.GetPieceBB<ROOK, us>(); pieces != 0;)
    {
        threats |= AttackBB<ROOK>(lsbpop(pieces), occ) & vulnerable;
    }

    return threats;
}

uint64_t ThreatMask(const BoardState& board, Side colour)
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
template void LegalMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void LegalMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void QuiescenceMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void QuiescenceMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void QuietMoves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void QuietMoves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);
