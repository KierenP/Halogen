#include "movegen/movegen.h"

#include <array>
#include <cassert>
#include <cstddef>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/list.h" // IWYU pragma: keep
#include "movegen/magic.h"
#include "movegen/move.h"
#include "utility/static_vector.h"

template <Side STM, typename T>
void add_loud_moves(const BoardState& board, T& moves, uint64_t pinned); // captures and/or promotions
template <Side STM, typename T>
void add_quiet_moves(const BoardState& board, T& moves, uint64_t pinned);

// Pawn moves
template <Side STM, typename T>
void pawn_pushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
void pawn_promotions(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
void pawn_double_pushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);
template <Side STM, typename T>
// Ep moves are always checked for legality so no need for pinned mask
void pawn_ep(const BoardState& board, T& moves);
template <Side STM, typename T>
void pawn_captures(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares = UNIVERSE);

// All other pieces
template <bool capture, Side STM, typename T>
void generate_knight_moves(const BoardState& board, T& moves, Square square);
template <bool capture, Side STM, typename T>
void generate_king_moves(const BoardState& board, T& moves, Square from);
template <PieceType piece, bool capture, Side STM, typename T>
void generate_sliding_moves(const BoardState& board, T& moves, Square square, uint64_t pinned, Square king);

// misc
template <Side STM, typename T>
void castle_moves(const BoardState& board, T& moves, uint64_t pinned);

// utility functions
template <Side STM>
bool move_puts_self_in_check(const BoardState& board, const Move& move);
template <Side STM>
bool ep_is_legal(const BoardState& board, const Move& move);
template <Side STM>
bool king_move_is_legal(const BoardState& board, const Move& move);
template <Side STM>
uint64_t pinned_bb(const BoardState& board);
// will tell you if the king WOULD be threatened on that square. Useful for finding defended / threatening pieces
template <Side colour>
bool is_square_threatened(const BoardState& board, Square square);
// colour is of the attacked piece! So to get the black threats of a white piece pass colour = WHITE!
template <Side colour>
uint64_t attacks_to_sq(const BoardState& board, Square square);
template <Side STM>
bool is_legal(const BoardState& board, const Move& move);

// special generators for when in check
template <bool capture, Side STM, typename T>
void king_evasions(const BoardState& board, T& moves, Square from, uint64_t threats);
// capture the attacker	(single threat only)
template <Side STM, typename T>
void capture_threat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned);
// block the attacker (single threat only)
template <Side STM, typename T>
void block_threat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned);

template <typename T>
void legal_moves(const BoardState& board, T& moves)
{
    loud_moves(board, moves);
    quiet_moves(board, moves);
}

template <typename T>
void loud_moves(const BoardState& board, T& moves)
{
    if (board.stm == WHITE)
    {
        return add_loud_moves<WHITE>(board, moves, pinned_bb<WHITE>(board));
    }
    else
    {
        return add_loud_moves<BLACK>(board, moves, pinned_bb<BLACK>(board));
    }
}

template <Side STM, typename T>
void add_loud_moves(const BoardState& board, T& moves, uint64_t pinned)
{
    const Square king = board.get_king_sq(STM);
    const uint64_t threats = attacks_to_sq<STM>(board, king);
    assert(popcount(threats) <= 2); // triple or more check is impossible

    if (popcount(threats) == 2)
    {
        // double check
        king_evasions<true, STM>(board, moves, king, threats);
    }
    else if (popcount(threats) == 1)
    {
        // single check
        pawn_captures<STM>(board, moves, pinned, threats);
        pawn_ep<STM>(board, moves);
        pawn_promotions<STM>(board, moves, pinned, BetweenBB[lsb(threats)][king]);
        king_evasions<true, STM>(board, moves, king, threats);
        capture_threat<STM>(board, moves, threats, pinned);
    }
    else
    {
        // not in check
        pawn_captures<STM>(board, moves, pinned);
        pawn_ep<STM>(board, moves);
        pawn_promotions<STM>(board, moves, pinned);
        generate_king_moves<true, STM>(board, moves, king);

        for (uint64_t pieces = board.get_pieces_bb(QUEEN, STM); pieces != 0;)
            generate_sliding_moves<QUEEN, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(ROOK, STM); pieces != 0;)
            generate_sliding_moves<ROOK, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(BISHOP, STM); pieces != 0;)
            generate_sliding_moves<BISHOP, true, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(KNIGHT, STM) & ~pinned; pieces != 0;)
            generate_knight_moves<true, STM>(board, moves, lsbpop(pieces));
    }
}

template <typename T>
void quiet_moves(const BoardState& board, T& moves)
{
    if (board.stm == WHITE)
    {
        return add_quiet_moves<WHITE>(board, moves, pinned_bb<WHITE>(board));
    }
    else
    {
        return add_quiet_moves<BLACK>(board, moves, pinned_bb<BLACK>(board));
    }
}

template <Side STM, typename T>
void add_quiet_moves(const BoardState& board, T& moves, uint64_t pinned)
{
    const Square king = board.get_king_sq(STM);
    const uint64_t threats = attacks_to_sq<STM>(board, king);
    assert(popcount(threats) <= 2); // triple or more check is impossible

    if (popcount(threats) == 2)
    {
        // double check
        king_evasions<false, STM>(board, moves, king, threats);
    }
    else if (popcount(threats) == 1)
    {
        // single check
        const auto block_squares = BetweenBB[lsb(threats)][king];
        pawn_pushes<STM>(board, moves, pinned, block_squares);
        pawn_double_pushes<STM>(board, moves, pinned, block_squares);
        king_evasions<false, STM>(board, moves, king, threats);
        block_threat<STM>(board, moves, threats, pinned);
    }
    else
    {
        pawn_pushes<STM>(board, moves, pinned);
        pawn_double_pushes<STM>(board, moves, pinned);
        castle_moves<STM>(board, moves, pinned);

        for (uint64_t pieces = board.get_pieces_bb(QUEEN, STM); pieces != 0;)
            generate_sliding_moves<QUEEN, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(ROOK, STM); pieces != 0;)
            generate_sliding_moves<ROOK, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(BISHOP, STM); pieces != 0;)
            generate_sliding_moves<BISHOP, false, STM>(board, moves, lsbpop(pieces), pinned, king);
        for (uint64_t pieces = board.get_pieces_bb(KNIGHT, STM) & ~pinned; pieces != 0;)
            generate_knight_moves<false, STM>(board, moves, lsbpop(pieces));

        generate_king_moves<false, STM>(board, moves, king);
    }
}

template <Side STM>
uint64_t pinned_bb(const BoardState& board)
{
    const Square king = board.get_king_sq(STM);
    const uint64_t all_pieces = board.get_pieces_bb();
    const uint64_t our_pieces = board.get_pieces_bb(STM);
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
    const auto bishops_and_queens = board.get_pieces_bb(BISHOP, !STM) | board.get_pieces_bb(QUEEN, !STM);
    const auto rooks_and_queens = board.get_pieces_bb(ROOK, !STM) | board.get_pieces_bb(QUEEN, !STM);
    check_for_pins(DiagonalBB[enum_to<Diagonal>(king)] & bishops_and_queens);
    check_for_pins(AntiDiagonalBB[enum_to<AntiDiagonal>(king)] & bishops_and_queens);
    check_for_pins(RankBB[enum_to<Rank>(king)] & rooks_and_queens);
    check_for_pins(FileBB[enum_to<File>(king)] & rooks_and_queens);

    pins |= SquareBB[king];

    return pins;
}

// Moves going from a square to squares on a bitboard
template <Side STM, typename T>
void append_legal_moves(Square from, uint64_t to, MoveFlag flag, T& moves)
{
    while (to != 0)
    {
        moves.emplace_back(from, lsbpop(to), flag);
    }
}

// Moves going to a square from squares on a bitboard
template <Side STM, typename T>
void append_legal_moves(uint64_t from, Square to, MoveFlag flag, T& moves)
{
    while (from != 0)
    {
        moves.emplace_back(lsbpop(from), to, flag);
    }
}

template <bool capture, Side STM, typename T>
void king_evasions(const BoardState& board, T& moves, Square from, uint64_t threats)
{
    const uint64_t occupied = board.get_pieces_bb();
    const auto flag = capture ? CAPTURE : QUIET;
    const auto sliding_threats = threats
        & (board.get_pieces_bb(QUEEN, !STM) | board.get_pieces_bb(ROOK, !STM) | board.get_pieces_bb(BISHOP, !STM));

    uint64_t targets = (capture ? board.get_pieces_bb(!STM) : ~occupied) & attack_bb<KING>(from, occupied);

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
        if (king_move_is_legal<STM>(board, move))
        {
            moves.emplace_back(move);
        }
    }
}

template <Side STM, typename T>
void capture_threat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned)
{
    const Square square = lsbpop(threats);

    const uint64_t potentialCaptures = attacks_to_sq<!STM>(board, square)
        & ~SquareBB[board.get_king_sq(STM)] // King captures handelled in GenerateKingMoves()
        & ~board.get_pieces_bb(PAWN, STM) // Pawn captures handelled elsewhere
        & ~pinned; // any pinned pieces cannot legally capture the threat

    append_legal_moves<STM>(potentialCaptures, square, CAPTURE, moves);
}

template <Side STM, typename T>
void block_threat(const BoardState& board, T& moves, uint64_t threats, uint64_t pinned)
{
    const Square threatSquare = lsbpop(threats);
    const Piece piece = board.get_square_piece(threatSquare);

    if (piece == WHITE_PAWN || piece == BLACK_PAWN || piece == WHITE_KNIGHT || piece == BLACK_KNIGHT)
        return; // cant block non-sliders. Also cant be threatened by enemy king

    uint64_t blockSquares = BetweenBB[threatSquare][board.get_king_sq(STM)];

    while (blockSquares != 0)
    {
        // pawn moves need to be handelled elsewhere because they might threaten a square without being able to move
        // there
        const Square square = lsbpop(blockSquares);
        // blocking moves are legal iff the piece is not pinned
        const uint64_t potentialBlockers
            = attacks_to_sq<!STM>(board, square) & ~board.get_pieces_bb(PAWN, STM) & ~pinned;
        append_legal_moves<STM>(potentialBlockers, square, QUIET, moves);
    }
}

template <Side STM, typename T>
void pawn_pushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.get_pieces_bb(PAWN, STM);
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.get_empty_bb() & target_squares;
    uint64_t pawnPushes = targets & ~(RankBB[RANK_1] | RankBB[RANK_8]); // pushes that aren't to the back ranks

    while (pawnPushes != 0)
    {
        const Square end = lsbpop(pawnPushes);
        const Square start = end - foward;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if (!(pinned & SquareBB[start]) || (enum_to<File>(start) == enum_to<File>(board.get_king_sq(STM))))
        {
            moves.emplace_back(start, end, QUIET);
        }
    }
}

template <Side STM, typename T>
void pawn_promotions(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    const uint64_t pawnSquares = board.get_pieces_bb(PAWN, STM);
    const uint64_t targets = shift_bb<foward>(pawnSquares) & board.get_empty_bb() & target_squares;
    uint64_t pawnPromotions = targets & (RankBB[RANK_1] | RankBB[RANK_8]); // pushes that are to the back ranks

    while (pawnPromotions != 0)
    {
        const Square end = lsbpop(pawnPromotions);
        const Square start = end - foward;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if ((pinned & SquareBB[start]) && (enum_to<File>(start) != enum_to<File>(board.get_king_sq(STM))))
            continue;

        moves.emplace_back(start, end, KNIGHT_PROMOTION);
        moves.emplace_back(start, end, ROOK_PROMOTION);
        moves.emplace_back(start, end, BISHOP_PROMOTION);
        moves.emplace_back(start, end, QUEEN_PROMOTION);
    }
}

template <Side STM, typename T>
void pawn_double_pushes(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift foward2 = STM == WHITE ? Shift::NN : Shift::SS;
    constexpr Shift foward = STM == WHITE ? Shift::N : Shift::S;
    constexpr uint64_t RankMask = STM == WHITE ? RankBB[RANK_2] : RankBB[RANK_7];
    const uint64_t pawnSquares = board.get_pieces_bb(PAWN, STM) & RankMask;

    uint64_t targets = 0;
    targets = shift_bb<foward>(pawnSquares) & board.get_empty_bb();
    targets = shift_bb<foward>(targets) & board.get_empty_bb() & target_squares;

    while (targets != 0)
    {
        const Square end = lsbpop(targets);
        const Square start = end - foward2;

        // If we are pinned, the move is legal iff we are on the same file as the king
        if (!(pinned & SquareBB[start]) || (enum_to<File>(start) == enum_to<File>(board.get_king_sq(STM))))
            moves.emplace_back(start, end, PAWN_DOUBLE_MOVE);
    }
}

template <Side STM, typename T>
void pawn_ep(const BoardState& board, T& moves)
{
    if (board.en_passant <= SQ_H8)
    {
        uint64_t potentialAttackers = PawnAttacks[!STM][board.en_passant] & board.get_pieces_bb(PAWN, STM);

        while (potentialAttackers != 0)
        {
            const Square source = lsbpop(potentialAttackers);
            const Move move(source, board.en_passant, EN_PASSANT);
            if (ep_is_legal<STM>(board, move))
                moves.emplace_back(move);
        }
    }
}

template <Side STM, typename T>
void pawn_captures(const BoardState& board, T& moves, uint64_t pinned, uint64_t target_squares)
{
    constexpr Shift fowardleft = STM == WHITE ? Shift::NW : Shift::SE;
    constexpr Shift fowardright = STM == WHITE ? Shift::NE : Shift::SW;
    const uint64_t pawnSquares = board.get_pieces_bb(PAWN, STM);

    const uint64_t leftAttack = shift_bb<fowardleft>(pawnSquares) & board.get_pieces_bb(!STM) & target_squares;
    const uint64_t rightAttack = shift_bb<fowardright>(pawnSquares) & board.get_pieces_bb(!STM) & target_squares;

    auto generate_attacks = [&](uint64_t attacks, Shift forward, auto get_cardinal)
    {
        while (attacks != 0)
        {
            const Square end = lsbpop(attacks);
            const Square start = end - forward;

            // If we are pinned, the move is legal iff we are on the same [anti]diagonal as the king
            if ((pinned & SquareBB[start]) && (get_cardinal(start) != get_cardinal(board.get_king_sq(STM))))
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
bool check_castle_move(
    const BoardState& board, Square king_start_sq, Square king_end_sq, Square rook_start_sq, Square rook_end_sq)
{
    const uint64_t blockers = board.get_pieces_bb() ^ SquareBB[king_start_sq] ^ SquareBB[rook_start_sq];

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
        if (is_square_threatened<STM>(board, sq))
        {
            return false;
        }
    }

    return true;
}

template <Side STM, typename T>
void castle_moves(const BoardState& board, T& moves, uint64_t pinned)
{
    // tricky edge case, if the rook is pinned then castling will put the king in check,
    // but it is possible none of the squares the king will move through will be threatened
    // before the rook is moved.
    uint64_t white_castle = board.castle_squares & RankBB[RANK_1] & ~pinned;

    while (STM == WHITE && white_castle)
    {
        const Square king_sq = board.get_king_sq(WHITE);
        const Square rook_sq = lsbpop(white_castle);

        if (king_sq > rook_sq && check_castle_move<STM>(board, king_sq, SQ_C1, rook_sq, SQ_D1))
        {
            moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
        }
        if (king_sq < rook_sq && check_castle_move<STM>(board, king_sq, SQ_G1, rook_sq, SQ_F1))
        {
            moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
        }
    }

    uint64_t black_castle = board.castle_squares & RankBB[RANK_8] & ~pinned;

    while (STM == BLACK && black_castle)
    {
        const Square king_sq = board.get_king_sq(BLACK);
        const Square rook_sq = lsbpop(black_castle);

        if (king_sq > rook_sq && check_castle_move<STM>(board, king_sq, SQ_C8, rook_sq, SQ_D8))
        {
            moves.emplace_back(king_sq, rook_sq, A_SIDE_CASTLE);
        }
        if (king_sq < rook_sq && check_castle_move<STM>(board, king_sq, SQ_G8, rook_sq, SQ_F8))
        {
            moves.emplace_back(king_sq, rook_sq, H_SIDE_CASTLE);
        }
    }
}

template <bool capture, Side STM, typename T>
void generate_knight_moves(const BoardState& board, T& moves, Square square)
{
    const uint64_t occupied = board.get_pieces_bb();
    const uint64_t targets = (capture ? board.get_pieces_bb(!STM) : ~occupied) & attack_bb<KNIGHT>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;
    append_legal_moves<STM>(square, targets, flag, moves);
}

template <PieceType piece, bool capture, Side STM, typename T>
void generate_sliding_moves(const BoardState& board, T& moves, Square square, uint64_t pinned, Square king)
{
    const uint64_t occupied = board.get_pieces_bb();
    const uint64_t targets = (capture ? board.get_pieces_bb(!STM) : ~occupied) & attack_bb<piece>(square, occupied);
    const auto flag = capture ? CAPTURE : QUIET;

    if (!(pinned & SquareBB[square]))
    {
        append_legal_moves<STM>(square, targets, flag, moves);
    }
    else
    {
        append_legal_moves<STM>(square, targets & RayBB[king][square], flag, moves);
    }
}

template <bool capture, Side STM, typename T>
void generate_king_moves(const BoardState& board, T& moves, Square from)
{
    const uint64_t occupied = board.get_pieces_bb();
    const auto flag = capture ? CAPTURE : QUIET;

    uint64_t targets = (capture ? board.get_pieces_bb(!STM) : ~occupied) & attack_bb<KING>(from, occupied);

    while (targets != 0)
    {
        const Square target = lsbpop(targets);
        const Move move(from, target, flag);
        if (king_move_is_legal<STM>(board, move))
        {
            moves.emplace_back(move);
        }
    }
}

template <Side colour>
bool is_square_threatened(const BoardState& board, Square square)
{
    if (attack_bb<KNIGHT>(square) & board.get_pieces_bb(KNIGHT, !colour))
        return true;
    if (PawnAttacks[colour][square] & board.get_pieces_bb(PAWN, !colour))
        return true;
    if (attack_bb<KING>(square) & board.get_pieces_bb(KING, !colour))
        return true;

    const uint64_t queens = board.get_pieces_bb(QUEEN, !colour);
    const uint64_t bishops = board.get_pieces_bb(BISHOP, !colour);
    const uint64_t rooks = board.get_pieces_bb(ROOK, !colour);
    const uint64_t occ = board.get_pieces_bb();

    if (attack_bb<BISHOP>(square, occ) & (bishops | queens))
        return true;
    if (attack_bb<ROOK>(square, occ) & (rooks | queens))
        return true;

    return false;
}

template <Side colour>
bool IsInCheck(const BoardState& board)
{
    return is_square_threatened<colour>(board, board.get_king_sq(colour));
}

bool is_in_check(const BoardState& board, Side colour)
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

bool is_in_check(const BoardState& board)
{
    return is_in_check(board, board.stm);
}

template <Side colour>
uint64_t attacks_to_sq(const BoardState& board, Square square)
{
    uint64_t threats = EMPTY;

    const uint64_t queens = board.get_pieces_bb(QUEEN, !colour);
    const uint64_t bishops = board.get_pieces_bb(BISHOP, !colour);
    const uint64_t rooks = board.get_pieces_bb(ROOK, !colour);
    const uint64_t occ = board.get_pieces_bb();

    threats |= (attack_bb<KNIGHT>(square) & board.get_pieces_bb(KNIGHT, !colour));
    threats |= (PawnAttacks[colour][square] & board.get_pieces_bb(PAWN, !colour));
    threats |= (attack_bb<KING>(square) & board.get_pieces_bb(KING, !colour));
    threats |= (attack_bb<BISHOP>(square, occ) & (bishops | queens));
    threats |= (attack_bb<ROOK>(square, occ) & (rooks | queens));

    return threats;
}

bool is_legal(const BoardState& board, const Move& move)
{
    if (board.stm == WHITE)
    {
        return is_legal<WHITE>(board, move);
    }
    else
    {
        return is_legal<BLACK>(board, move);
    }
}

template <Side STM>
bool is_legal(const BoardState& board, const Move& move)
{
    /*Obvious check first*/
    if (move == Move::Uninitialized)
        return false;

    const Piece piece = board.get_square_piece(move.from());

    /*Make sure there's a piece to be moved*/
    if (board.get_square_piece(move.from()) == N_PIECES)
        return false;

    /*Make sure the piece are are moving is ours*/
    if (enum_to<Side>(piece) != STM)
        return false;

    /*Make sure we aren't capturing our own piece - except when castling it's ok (chess960)*/
    if (!move.is_castle() && board.get_square_piece(move.to()) != N_PIECES
        && enum_to<Side>(board.get_square_piece(move.to())) == STM)
        return false;

    /*We don't use these flags*/
    if (move.flag() == DONT_USE_1 || move.flag() == DONT_USE_2)
        return false;

    const uint64_t allPieces = board.get_pieces_bb();

    /*Anything in the way of sliding pieces?*/
    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP || piece == WHITE_ROOK || piece == BLACK_ROOK
        || piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if (!path_clear(move.from(), move.to(), allPieces))
            return false;
    }

    /*Pawn moves are complex*/
    if (enum_to<PieceType>(piece) == PAWN)
    {
        const int forward = piece == WHITE_PAWN ? 1 : -1;
        const Rank startingRank = piece == WHITE_PAWN ? RANK_2 : RANK_7;

        // pawn push
        if (rank_diff(move.to(), move.from()) == forward && file_diff(move.from(), move.to()) == 0)
        {
            if (board.is_occupied(move.to())) // Something in the way!
                return false;
        }

        // pawn double push
        else if (rank_diff(move.to(), move.from()) == forward * 2 && file_diff(move.from(), move.to()) == 0)
        {
            if (enum_to<Rank>(move.from()) != startingRank) // double move not from starting rank
                return false;
            if (board.is_occupied(move.to())) // something on target square
                return false;
            if (!board.is_empty(static_cast<Square>((move.to() + move.from()) / 2))) // something in between
                return false;
        }

        // pawn capture (not EP)
        else if (rank_diff(move.to(), move.from()) == forward && abs_file_diff(move.from(), move.to()) == 1
            && board.en_passant != move.to())
        {
            if (board.is_empty(move.to())) // nothing there to capture
                return false;
        }

        // pawn capture (EP)
        else if (rank_diff(move.to(), move.from()) == forward && abs_file_diff(move.from(), move.to()) == 1
            && board.en_passant == move.to())
        {
            if (board.is_empty(
                    get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from())))) // nothing there to capture
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
        if ((SquareBB[move.to()] & KnightAttacks[move.from()]) == 0)
            return false;
    }

    if ((piece == WHITE_KING || piece == BLACK_KING) && !move.is_castle())
    {
        if ((SquareBB[move.to()] & KingAttacks[move.from()]) == 0)
            return false;
    }

    if (piece == WHITE_ROOK || piece == BLACK_ROOK)
    {
        if ((SquareBB[move.to()] & RookAttacks[move.from()]) == 0)
            return false;
    }

    if (piece == WHITE_BISHOP || piece == BLACK_BISHOP)
    {
        if ((SquareBB[move.to()] & BishopAttacks[move.from()]) == 0)
            return false;
    }

    if (piece == WHITE_QUEEN || piece == BLACK_QUEEN)
    {
        if ((SquareBB[move.to()] & QueenAttacks[move.from()]) == 0)
            return false;
    }

    /*For castle moves, just generate them and see if we find a match*/
    if (move.flag() == A_SIDE_CASTLE || move.flag() == H_SIDE_CASTLE)
    {
        StaticVector<Move, 4> moves;
        castle_moves<STM>(board, moves, pinned_bb<STM>(board));
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
    if (board.is_occupied(move.to()))
        flag = CAPTURE;

    // Double pawn moves
    if (abs_rank_diff(move.from(), move.to()) == 2)
        if (board.get_square_piece(move.from()) == WHITE_PAWN || board.get_square_piece(move.from()) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    // En passant
    if (move.to() == board.en_passant)
        if (board.get_square_piece(move.from()) == WHITE_PAWN || board.get_square_piece(move.from()) == BLACK_PAWN)
            flag = EN_PASSANT;

    // Promotion
    if ((board.get_square_piece(move.from()) == WHITE_PAWN && enum_to<Rank>(move.to()) == RANK_8)
        || (board.get_square_piece(move.from()) == BLACK_PAWN && enum_to<Rank>(move.to()) == RANK_1))
    {
        if (board.is_occupied(move.to()))
        {
            if (move.flag() != KNIGHT_PROMOTION_CAPTURE && move.flag() != ROOK_PROMOTION_CAPTURE
                && move.flag() != QUEEN_PROMOTION_CAPTURE && move.flag() != BISHOP_PROMOTION_CAPTURE)
                return false;
        }
        else
        {
            if (move.flag() != KNIGHT_PROMOTION && move.flag() != ROOK_PROMOTION && move.flag() != QUEEN_PROMOTION
                && move.flag() != BISHOP_PROMOTION)
                return false;
        }
    }
    else
    {
        // check the decided on flag matches
        if (flag != move.flag())
            return false;
    }
    //-----------------------------

    /*Move puts me in check*/
    if (move_puts_self_in_check<STM>(board, move))
        return false;

    return true;
}

/*
This function does not work for casteling moves. They are tested for legality their creation.
*/
template <Side STM>
bool move_puts_self_in_check(const BoardState& board, const Move& move)
{
    assert(move.flag() != A_SIDE_CASTLE);
    assert(move.flag() != H_SIDE_CASTLE);

    const Square king
        = enum_to<PieceType>(board.get_square_piece(move.from())) == KING ? move.to() : board.get_king_sq(STM);

    const uint64_t knights = board.get_pieces_bb(KNIGHT, !STM) & ~SquareBB[move.to()];
    if (attack_bb<KNIGHT>(king) & knights)
        return true;

    uint64_t pawns = board.get_pieces_bb(PAWN, !STM) & ~SquareBB[move.to()];

    if (move.flag() == EN_PASSANT)
        pawns &= ~SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];

    if (PawnAttacks[STM][king] & pawns)
        return true;

    if (attack_bb<KING>(king) & board.get_pieces_bb(KING, !STM))
        return true;

    const uint64_t queens = board.get_pieces_bb(QUEEN, !STM) & ~SquareBB[move.to()];
    const uint64_t bishops = board.get_pieces_bb(BISHOP, !STM) & ~SquareBB[move.to()];
    const uint64_t rooks = board.get_pieces_bb(ROOK, !STM) & ~SquareBB[move.to()];
    uint64_t occ = board.get_pieces_bb();

    occ &= ~SquareBB[move.from()];
    occ |= SquareBB[move.to()];

    if (move.flag() == EN_PASSANT)
        occ &= ~SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];

    if (attack_bb<BISHOP>(king, occ) & (bishops | queens))
        return true;
    if (attack_bb<ROOK>(king, occ) & (rooks | queens))
        return true;

    return false;
}

bool ep_is_legal(const BoardState& board, const Move& move)
{
    if (board.stm == WHITE)
    {
        return ep_is_legal<WHITE>(board, move);
    }
    else
    {
        return ep_is_legal<BLACK>(board, move);
    }
}

template <Side STM>
bool ep_is_legal(const BoardState& board, const Move& move)
{
    const Square king = board.get_king_sq(STM);

    if (attack_bb<KNIGHT>(king) & board.get_pieces_bb(KNIGHT, !STM))
        return false;

    if (attack_bb<KING>(king) & board.get_pieces_bb(KING, !STM))
        return false;

    const uint64_t pawns
        = board.get_pieces_bb(PAWN, !STM) & ~SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];
    if (PawnAttacks[STM][king] & pawns)
        return false;

    const uint64_t queens = board.get_pieces_bb(QUEEN, !STM) & ~SquareBB[move.to()];
    const uint64_t bishops = board.get_pieces_bb(BISHOP, !STM) & ~SquareBB[move.to()];
    const uint64_t rooks = board.get_pieces_bb(ROOK, !STM) & ~SquareBB[move.to()];
    uint64_t occ = board.get_pieces_bb();

    occ &= ~SquareBB[move.from()];
    occ &= ~SquareBB[get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()))];
    occ |= SquareBB[move.to()];

    if (attack_bb<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (attack_bb<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <Side STM>
bool king_move_is_legal(const BoardState& board, const Move& move)
{
    const Square king = move.to();

    if (attack_bb<KNIGHT>(king) & board.get_pieces_bb(KNIGHT, !STM))
        return false;

    if (PawnAttacks[STM][king] & board.get_pieces_bb(PAWN, !STM))
        return false;

    if (attack_bb<KING>(king) & board.get_pieces_bb(KING, !STM))
        return false;

    const uint64_t queens = board.get_pieces_bb(QUEEN, !STM) & ~SquareBB[move.to()];
    const uint64_t bishops = board.get_pieces_bb(BISHOP, !STM) & ~SquareBB[move.to()];
    const uint64_t rooks = board.get_pieces_bb(ROOK, !STM) & ~SquareBB[move.to()];
    uint64_t occ = board.get_pieces_bb();

    // where this function is called, we already filter out attacks going through the 'from' square, so we don't need to
    // update occ to apply the king move

    if (attack_bb<BISHOP>(king, occ) & (bishops | queens))
        return false;
    if (attack_bb<ROOK>(king, occ) & (rooks | queens))
        return false;

    return true;
}

template <>
uint64_t attack_bb<KNIGHT>(Square sq, uint64_t)
{
    return KnightAttacks[sq];
}

template <>
uint64_t attack_bb<BISHOP>(Square sq, uint64_t occupied)
{
    return Magic::bishopTable.attack_mask(sq, occupied);
}

template <>
uint64_t attack_bb<ROOK>(Square sq, uint64_t occupied)
{
    return Magic::rookTable.attack_mask(sq, occupied);
}

template <>
uint64_t attack_bb<QUEEN>(Square sq, uint64_t occupied)
{
    return attack_bb<ROOK>(sq, occupied) | attack_bb<BISHOP>(sq, occupied);
}

template <>
uint64_t attack_bb<KING>(Square sq, uint64_t)
{
    return KingAttacks[sq];
}

std::array<uint64_t, N_PIECE_TYPES> capture_threat_mask(const BoardState& board, Side colour)
{
    std::array<uint64_t, N_PIECE_TYPES> threats = {};
    const uint64_t occ = board.get_pieces_bb();

    uint64_t attacks = EMPTY;

    // pawn threats
    for (uint64_t pieces = board.get_pieces_bb(PAWN, colour); pieces != 0;)
    {
        attacks |= PawnAttacks[colour][lsbpop(pieces)];
    }

    threats[KNIGHT] = attacks;

    // knight threats
    for (uint64_t pieces = board.get_pieces_bb(KNIGHT, colour); pieces != 0;)
    {
        attacks |= attack_bb<KNIGHT>(lsbpop(pieces), occ);
    }

    threats[BISHOP] = attacks;

    // bishop threats
    for (uint64_t pieces = board.get_pieces_bb(BISHOP, colour); pieces != 0;)
    {
        attacks |= attack_bb<BISHOP>(lsbpop(pieces), occ);
    }

    threats[ROOK] = attacks;

    // rook threats
    for (uint64_t pieces = board.get_pieces_bb(ROOK, colour); pieces != 0;)
    {
        attacks |= attack_bb<ROOK>(lsbpop(pieces), occ);
    }

    threats[QUEEN] = attacks;

    // queen threats
    for (uint64_t pieces = board.get_pieces_bb(QUEEN, colour); pieces != 0;)
    {
        attacks |= attack_bb<QUEEN>(lsbpop(pieces), occ);
    }

    threats[KING] = attacks;

    return threats;
}

// Explicit template instantiation
template void legal_moves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void legal_moves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void loud_moves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void loud_moves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);

template void quiet_moves<ExtendedMoveList>(const BoardState& board, ExtendedMoveList& moves);
template void quiet_moves<BasicMoveList>(const BoardState& board, BasicMoveList& moves);
