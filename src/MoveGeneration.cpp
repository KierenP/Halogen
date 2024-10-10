#include "MoveGeneration.h"

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <vector>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Magic.h"
#include "Move.h"
#include "MoveList.h"
#include "Utility.h"

template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, BB pinned); // captures and/or promotions
template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, BB pinned);

// Pawn moves
template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, BB pinned, BB target_squares = BB::all);
template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, BB pinned, BB target_squares = BB::all);
template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, BB pinned, BB target_squares = BB::all);
template <Players STM, typename T>
// Ep moves are always checked for legality so no need for pinned mask
void PawnEnPassant(const BoardState& board, T& moves);
template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, BB pinned, BB target_squares = BB::all);

// All other pieces
template <PieceTypes pieceType, bool capture, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, BB pinned, Square king);

// misc
template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves, BB pinned);

// utility functions
template <Players STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move);
template <Players STM>
BB PinnedMask(const BoardState& board);
// will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square);
// colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!
template <Players colour>
BB GetThreats(const BoardState& board, Square square);
template <Players STM>
bool MoveIsLegal(const BoardState& board, const Move& move);

// special generators for when in check
template <Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves); // move the king out of danger	(single or multi threat)
template <Players STM, typename T>
void KingCapturesEvade(const BoardState& board, T& moves); // use only for multi threat with king evasions
// capture the attacker	(single threat only)
template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, BB threats, BB pinned);
// block the attacker (single threat only)
template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, BB threats, BB pinned);

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

template <Players STM, typename T>
void AddQuiescenceMoves(const BoardState& board, T& moves, BB pinned)
{
    const Square king = board.GetKing(STM);
    BB threats = GetThreats<STM>(board, king);
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    if (GetBitCount(threats) == 2)
    {
        // double check
        KingCapturesEvade<STM>(board, moves);
    }
    else if (GetBitCount(threats) == 1)
    {
        // single check
        PawnCaptures<STM>(board, moves, pinned, threats);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned, betweenArray[LSB(threats)][king]);
        KingCapturesEvade<STM>(board, moves);
        CaptureThreat<STM>(board, moves, threats, pinned);
    }
    else
    {
        // not in check
        PawnCaptures<STM>(board, moves, pinned);
        PawnEnPassant<STM>(board, moves);
        PawnPromotions<STM>(board, moves, pinned);

        extract_bits(board.GetPieceBB<KNIGHT, STM>(),
            [&](auto sq) { GenerateMoves<KNIGHT, true, STM>(board, moves, sq, pinned, king); });

        extract_bits(board.GetPieceBB<BISHOP, STM>(),
            [&](auto sq) { GenerateMoves<BISHOP, true, STM>(board, moves, sq, pinned, king); });

        KingCapturesEvade<STM>(board, moves);

        extract_bits(board.GetPieceBB<ROOK, STM>(),
            [&](auto sq) { GenerateMoves<ROOK, true, STM>(board, moves, sq, pinned, king); });

        extract_bits(board.GetPieceBB<QUEEN, STM>(),
            [&](auto sq) { GenerateMoves<QUEEN, true, STM>(board, moves, sq, pinned, king); });
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

template <Players STM, typename T>
void AddQuietMoves(const BoardState& board, T& moves, BB pinned)
{
    const Square king = board.GetKing(STM);
    BB threats = GetThreats<STM>(board, king);
    assert(GetBitCount(threats) <= 2); // triple or more check is impossible

    if (GetBitCount(threats) == 2)
    {
        // double check
        KingEvasions<STM>(board, moves);
    }
    else if (GetBitCount(threats) == 1)
    {
        // single check
        const auto block_squares = betweenArray[LSB(threats)][king];
        PawnPushes<STM>(board, moves, pinned, block_squares);
        PawnDoublePushes<STM>(board, moves, pinned, block_squares);
        KingEvasions<STM>(board, moves);
        BlockThreat<STM>(board, moves, threats, pinned);
    }
    else
    {
        PawnPushes<STM>(board, moves, pinned);
        PawnDoublePushes<STM>(board, moves, pinned);
        CastleMoves<STM>(board, moves, pinned);

        extract_bits(board.GetPieceBB<KNIGHT, STM>(),
            [&](auto sq) { GenerateMoves<KNIGHT, false, STM>(board, moves, sq, pinned, king); });
        extract_bits(board.GetPieceBB<BISHOP, STM>(),
            [&](auto sq) { GenerateMoves<BISHOP, false, STM>(board, moves, sq, pinned, king); });
        extract_bits(board.GetPieceBB<QUEEN, STM>(),
            [&](auto sq) { GenerateMoves<QUEEN, false, STM>(board, moves, sq, pinned, king); });
        extract_bits(board.GetPieceBB<ROOK, STM>(),
            [&](auto sq) { GenerateMoves<ROOK, false, STM>(board, moves, sq, pinned, king); });

        KingEvasions<STM>(board, moves);
    }
}

template <Players STM>
BB PinnedMask(const BoardState& board)
{
    Square king = board.GetKing(STM);
    BB pins = BB::none;
    BB all_pieces = board.GetAllPieces();
    BB our_pieces = board.GetPieces<STM>();

    auto check_for_pins = [&](BB threats)
    {
        extract_bits(threats,
            [&](auto threat_sq)
            {
                // get the pieces standing in between the king and the threat
                BB possible_pins = betweenArray[king][threat_sq] & all_pieces;

                // if there is just one piece and it's ours it's pinned
                if (GetBitCount(possible_pins) == 1 && (possible_pins & our_pieces) != BB::none)
                {
                    pins |= SquareBB[LSBpop(possible_pins)];
                }
            });
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
void AppendLegalMoves(Square from, BB to, MoveFlag flag, T& moves)
{
    extract_bits(to, [&](auto sq) { moves.emplace_back(from, sq, flag); });
}

template <Players STM, typename T>
void AppendLegalMovesCheckLegality(Square from, BB to, const BoardState& board, MoveFlag flag, T& moves, BB pinned)
{
    extract_bits(to,
        [&](auto target)
        {
            Move move(from, target, flag);
            if (!pinned.contains(from) || !MovePutsSelfInCheck<STM>(board, move))
                moves.emplace_back(move);
        });
}

// Moves going to a square from squares on a bitboard
template <Players STM, typename T>
void AppendLegalMoves(BB from, Square to, MoveFlag flag, T& moves)
{
    extract_bits(from, [&](auto sq) { moves.emplace_back(sq, to, flag); });
}

template <Players STM, typename T>
void AppendLegalMovesCheckLegality(BB from, Square to, const BoardState& board, MoveFlag flag, T& moves, BB pinned)
{
    extract_bits(from,
        [&](auto source)
        {
            Move move(source, to, flag);
            if (!pinned.contains(source) || !MovePutsSelfInCheck<STM>(board, move))
                moves.emplace_back(move);
        });
}

template <Players STM, typename T>
void KingEvasions(const BoardState& board, T& moves)
{
    Square square = board.GetKing(STM);
    BB quiets = board.GetEmptySquares() & KingAttacks[square];
    AppendLegalMovesCheckLegality<STM>(square, quiets, board, QUIET, moves, BB::all);
}

template <Players STM, typename T>
void KingCapturesEvade(const BoardState& board, T& moves)
{
    Square square = board.GetKing(STM);
    BB captures = board.GetPieces<!STM>() & KingAttacks[square];
    AppendLegalMovesCheckLegality<STM>(square, captures, board, CAPTURE, moves, BB::all);
}

template <Players STM, typename T>
void CaptureThreat(const BoardState& board, T& moves, BB threats, BB pinned)
{
    Square square = LSBpop(threats);

    BB potentialCaptures = GetThreats<!STM>(board, square)
        & ~SquareBB[board.GetKing(STM)] // King captures handelled in KingCapturesEvade()
        & ~board.GetPieceBB<PAWN, STM>() // Pawn captures handelled elsewhere
        & ~pinned; // any pinned pieces cannot legally capture the threat

    AppendLegalMoves<STM>(potentialCaptures, square, CAPTURE, moves);
}

template <Players STM, typename T>
void BlockThreat(const BoardState& board, T& moves, BB threats, BB pinned)
{
    Square threatSquare = LSBpop(threats);
    Pieces piece = board.GetSquare(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; // cant block non-sliders. Also cant be threatened by enemy king

    BB blockSquares = betweenArray[threatSquare][board.GetKing(STM)];

    extract_bits(blockSquares,
        [&](auto sq)
        {
            // pawn moves need to be handelled elsewhere because they might threaten a square without being able to move
            // there
            // blocking moves are legal iff the piece is not pinned
            BB potentialBlockers = GetThreats<!STM>(board, sq) & ~board.GetPieceBB<PAWN, STM>() & ~pinned;
            AppendLegalMoves<STM>(potentialBlockers, sq, QUIET, moves);
        });
}

template <Players STM, typename T>
void PawnPushes(const BoardState& board, T& moves, BB pinned, BB target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    BB targets = BB::none;
    BB pawnSquares = board.GetPieceBB<PAWN, STM>();
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    BB pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); // pushes that aren't to the back ranks

    extract_bits(pawnPushes,
        [&](auto end)
        {
            Square start = end - foward;

            // If we are pinned, the move is legal iff we are on the same file as the king
            if (!pinned.contains(start) || (GetFile(start) == GetFile(board.GetKing(STM))))
            {
                moves.emplace_back(start, end, QUIET);
            }
        });
}

template <Players STM, typename T>
void PawnPromotions(const BoardState& board, T& moves, BB pinned, BB target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    BB targets = BB::none;
    BB pawnSquares = board.GetPieceBB<PAWN, STM>();
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares() & target_squares;
    BB pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); // pushes that are to the back ranks

    extract_bits(pawnPromotions,
        [&](auto end)
        {
            Square start = end - foward;

            // If we are pinned, the move is legal iff we are on the same file as the king
            if (pinned.contains(start) && (GetFile(start) != GetFile(board.GetKing(STM))))
                return;

            moves.emplace_back(start, end, KNIGHT_PROMOTION);
            moves.emplace_back(start, end, ROOK_PROMOTION);
            moves.emplace_back(start, end, BISHOP_PROMOTION);
            moves.emplace_back(start, end, QUEEN_PROMOTION);
        });
}

template <Players STM, typename T>
void PawnDoublePushes(const BoardState& board, T& moves, BB pinned, BB target_squares)
{
    constexpr Shift foward2 = STM == WHITE ? Shift::NN : Shift::SS;
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    constexpr BB RankMask = STM == WHITE ? RankBB[RANK_2] : RankBB[RANK_7];
    BB targets = BB::none;
    BB pawnSquares = board.GetPieceBB<PAWN, STM>();

    pawnSquares &= RankMask;
    targets = shift_bb<foward>(pawnSquares) & board.GetEmptySquares();
    targets = shift_bb<foward>(targets) & board.GetEmptySquares() & target_squares;

    extract_bits(targets,
        [&](auto end)
        {
            Square start = end - foward2;

            // If we are pinned, the move is legal iff we are on the same file as the king
            if (!pinned.contains(start) || (GetFile(start) == GetFile(board.GetKing(STM))))
                moves.emplace_back(start, end, PAWN_DOUBLE_MOVE);
        });
}

template <Players STM, typename T>
void PawnEnPassant(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        BB potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.GetPieceBB<PAWN, STM>();
        AppendLegalMovesCheckLegality<STM>(potentialAttackers, board.en_passant, board, EN_PASSANT, moves, BB::all);
    }
}

template <Players STM, typename T>
void PawnCaptures(const BoardState& board, T& moves, BB pinned, BB target_squares)
{
    constexpr Shift fowardleft = STM == WHITE ? Shift::NW : Shift::SE;
    constexpr Shift fowardright = STM == WHITE ? Shift::NE : Shift::SW;
    BB leftAttack = BB::none;
    BB rightAttack = BB::none;
    BB pawnSquares = board.GetPieceBB<PAWN, STM>();
    leftAttack = shift_bb<fowardleft>(pawnSquares) & board.GetPieces<!STM>() & target_squares;
    rightAttack = shift_bb<fowardright>(pawnSquares) & board.GetPieces<!STM>() & target_squares;

    auto generate_attacks = [&](BB attacks, Shift forward, auto get_cardinal)
    {
        extract_bits(attacks,
            [&](auto end)
            {
                Square start = end - forward;

                // If we are pinned, the move is legal iff we are on the same [anti]diagonal as the king
                if (pinned.contains(start) && (get_cardinal(start) != get_cardinal(board.GetKing(STM))))
                    return;

                if (GetRank(end) == RANK_1 || GetRank(end) == RANK_8)
                {
                    moves.emplace_back(start, end, KNIGHT_PROMOTION_CAPTURE);
                    moves.emplace_back(start, end, ROOK_PROMOTION_CAPTURE);
                    moves.emplace_back(start, end, BISHOP_PROMOTION_CAPTURE);
                    moves.emplace_back(start, end, QUEEN_PROMOTION_CAPTURE);
                }
                else
                    moves.emplace_back(start, end, CAPTURE);
            });
    };

    // TODO: this was added to keep the bench the same
    if constexpr (STM == WHITE)
    {
        generate_attacks(leftAttack, fowardleft, GetAntiDiagonal);
        generate_attacks(rightAttack, fowardright, GetDiagonal);
    }
    else
    {
        generate_attacks(rightAttack, fowardright, GetDiagonal);
        generate_attacks(leftAttack, fowardleft, GetAntiDiagonal);
    }
}

template <Players STM>
bool CheckCastleMove(
    const BoardState& board, Square king_start_sq, Square king_end_sq, Square rook_start_sq, Square rook_end_sq)
{
    BB blockers = board.GetAllPieces() ^ SquareBB[king_start_sq] ^ SquareBB[rook_start_sq];

    if (((betweenArray[rook_start_sq][rook_end_sq] | SquareBB[rook_end_sq]) & blockers) != BB::none)
    {
        return false;
    }

    if (((betweenArray[king_start_sq][king_end_sq] | SquareBB[king_end_sq]) & blockers) != BB::none)
    {
        return false;
    }

    BB king_path = betweenArray[king_start_sq][king_end_sq] | SquareBB[king_start_sq] | SquareBB[king_end_sq];

    if (any_of(king_path, [&](auto sq) { return IsSquareThreatened<STM>(board, sq); }))
    {
        return false;
    }

    return true;
}

template <Players STM, typename T>
void CastleMoves(const BoardState& board, T& moves, BB pinned)
{
    constexpr auto back_rank = STM == WHITE ? RANK_1 : RANK_8;
    constexpr auto a_king = STM == WHITE ? SQ_C1 : SQ_C8;
    constexpr auto a_rook = STM == WHITE ? SQ_D1 : SQ_D8;
    constexpr auto h_king = STM == WHITE ? SQ_G1 : SQ_G8;
    constexpr auto h_rook = STM == WHITE ? SQ_F1 : SQ_F8;

    // tricky edge case, if the rook is pinned then castling will put the king in check,
    // but it is possible none of the squares the king will move through will be threatened
    // before the rook is moved.
    BB castle_bb = board.castle_squares & RankBB[back_rank] & ~pinned;

    extract_bits(castle_bb,
        [&](auto rook_sq)
        {
            Square king_sq = board.GetKing(STM);

            if (king_sq > rook_sq && CheckCastleMove<STM>(board, king_sq, a_king, rook_sq, a_rook))
            {
                moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
            }
            if (king_sq < rook_sq && CheckCastleMove<STM>(board, king_sq, h_king, rook_sq, h_rook))
            {
                moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
            }
        });
}

template <PieceTypes pieceType, bool capture, Players STM, typename T>
void GenerateMoves(const BoardState& board, T& moves, Square square, BB pinned, Square king)
{
    BB occupied = board.GetAllPieces();
    BB targets = (capture ? board.GetPieces<!STM>() : ~occupied) & AttackBB<pieceType>(square, occupied);

    // If the piece is pinned, the move is legal iff it remains on the same rank/file/etc it was originally on shared
    // with the king

    if (!pinned.contains(square))
    {
    }
    else if (GetRank(king) == GetRank(square))
    {
        targets &= RankBB[GetRank(square)];
    }
    else if (GetFile(king) == GetFile(square))
    {
        targets &= FileBB[GetFile(square)];
    }
    else if (GetDiagonal(king) == GetDiagonal(square))
    {
        targets &= DiagonalBB[GetDiagonal(square)];
    }
    else if (GetAntiDiagonal(king) == GetAntiDiagonal(square))
    {
        targets &= AntiDiagonalBB[GetAntiDiagonal(square)];
    }

    AppendLegalMoves<STM>(square, targets, capture ? CAPTURE : QUIET, moves);

    return;
}

template <Players colour>
bool IsSquareThreatened(const BoardState& board, Square square)
{
    if ((AttackBB<KNIGHT>(square) & board.GetPieceBB<KNIGHT, !colour>()) != BB::none)
        return true;
    if ((PawnAttacks[colour][square] & board.GetPieceBB<PAWN, !colour>()) != BB::none)
        return true;
    if ((AttackBB<KING>(square) & board.GetPieceBB<KING, !colour>()) != BB::none)
        return true;

    BB queens = board.GetPieceBB<QUEEN, !colour>();
    BB bishops = board.GetPieceBB<BISHOP, !colour>();
    BB rooks = board.GetPieceBB<ROOK, !colour>();
    BB occ = board.GetAllPieces();

    if ((AttackBB<BISHOP>(square, occ) & (bishops | queens)) != BB::none)
        return true;
    if ((AttackBB<ROOK>(square, occ) & (rooks | queens)) != BB::none)
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
BB GetThreats(const BoardState& board, Square square)
{
    BB threats = BB::none;

    BB queens = board.GetPieceBB<QUEEN, !colour>();
    BB bishops = board.GetPieceBB<BISHOP, !colour>();
    BB rooks = board.GetPieceBB<ROOK, !colour>();
    BB occ = board.GetAllPieces();

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

    BB allPieces = board.GetAllPieces();

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
        if (!KnightAttacks[move.GetFrom()].contains(move.GetTo()))
            return false;
    }

    if ((piece == WHITE_KING || piece == BLACK_KING) && !move.IsCastle())
    {
        if (!KingAttacks[move.GetFrom()].contains(move.GetTo()))
            return false;
    }

    if (piece == WHITE_ROOK || piece == BLACK_ROOK)
    {
        if (!RookAttacks[move.GetFrom()].contains(move.GetTo()))
            return false;
    }

    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP)
    {
        if (!BishopAttacks[move.GetFrom()].contains(move.GetTo()))
            return false;
    }

    if (piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if (!QueenAttacks[move.GetFrom()].contains(move.GetTo()))
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

bool MovePutsSelfInCheck(const BoardState& board, const Move& move)
{
    if (board.stm == WHITE)
    {
        return MovePutsSelfInCheck<WHITE>(board, move);
    }
    else
    {
        return MovePutsSelfInCheck<BLACK>(board, move);
    }
}

/*
This function does not work for casteling moves. They are tested for legality their creation.
*/
template <Players STM>
bool MovePutsSelfInCheck(const BoardState& board, const Move& move)
{
    assert(move.GetFlag() != A_SIDE_CASTLE);
    assert(move.GetFlag() != H_SIDE_CASTLE);

    Square king = GetPieceType(board.GetSquare(move.GetFrom())) == KING ? move.GetTo() : board.GetKing(STM);

    BB knights = board.GetPieceBB<KNIGHT, !STM>() & ~SquareBB[move.GetTo()];
    if ((AttackBB<KNIGHT>(king) & knights) != BB::none)
        return true;

    BB pawns = board.GetPieceBB<PAWN, !STM>() & ~SquareBB[move.GetTo()];

    if (move.GetFlag() == EN_PASSANT)
        pawns &= ~SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];

    if ((PawnAttacks[STM][king] & pawns) != BB::none)
        return true;

    if ((AttackBB<KING>(king) & board.GetPieceBB<KING, !STM>()) != BB::none)
        return true;

    BB queens = board.GetPieceBB<QUEEN, !STM>() & ~SquareBB[move.GetTo()];
    BB bishops = board.GetPieceBB<BISHOP, !STM>() & ~SquareBB[move.GetTo()];
    BB rooks = board.GetPieceBB<ROOK, !STM>() & ~SquareBB[move.GetTo()];
    BB occ = board.GetAllPieces();

    occ &= ~SquareBB[move.GetFrom()];
    occ |= SquareBB[move.GetTo()];

    if (move.GetFlag() == EN_PASSANT)
        occ &= ~SquareBB[GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom()))];

    if ((AttackBB<BISHOP>(king, occ) & (bishops | queens)) != BB::none)
        return true;
    if ((AttackBB<ROOK>(king, occ) & (rooks | queens)) != BB::none)
        return true;

    return false;
}

template <Players us>
BB ThreatMask(const BoardState& board)
{
    const BB occ = board.GetAllPieces();

    BB threats = BB::none;
    BB vulnerable = board.GetPieces<!us>();

    // Pawn capture non-pawn
    vulnerable ^= board.GetPieceBB<PAWN, !us>();
    extract_bits(board.GetPieceBB<PAWN, us>(), [&](auto sq) { threats |= PawnAttacks[us][sq] & vulnerable; });

    // Bishop/Knight capture Rook/Queen
    vulnerable ^= board.GetPieceBB<KNIGHT, !us>() | board.GetPieceBB<BISHOP, !us>();
    extract_bits(board.GetPieceBB<KNIGHT, us>(), [&](auto sq) { threats |= AttackBB<KNIGHT>(sq, occ) & vulnerable; });
    extract_bits(board.GetPieceBB<BISHOP, us>(), [&](auto sq) { threats |= AttackBB<BISHOP>(sq, occ) & vulnerable; });

    // Rook capture queen
    vulnerable ^= board.GetPieceBB<ROOK, !us>();
    extract_bits(board.GetPieceBB<ROOK, us>(), [&](auto sq) { threats |= AttackBB<ROOK>(sq, occ) & vulnerable; });

    return threats;
}

BB ThreatMask(const BoardState& board, Players colour)
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

// Below code adapted with permission from Terje, author of Weiss.
//--------------------------------------------------------------------------

// Returns the attack bitboard for a piece of piecetype on square sq
template <PieceTypes pieceType>
BB AttackBB(Square sq, BB occupied)
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

template BB AttackBB<KNIGHT>(Square sq, BB occupied);
template BB AttackBB<BISHOP>(Square sq, BB occupied);
template BB AttackBB<ROOK>(Square sq, BB occupied);
template BB AttackBB<QUEEN>(Square sq, BB occupied);
template BB AttackBB<KING>(Square sq, BB occupied);
