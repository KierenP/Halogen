#include "StagedMoveGenerator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

#include "BitBoardDefine.h"
#include "GameState.h"
#include "MoveGeneration.h"
#include "SearchData.h"
#include "TTEntry.h"
#include "TranspositionTable.h"
#include "Zobrist.h"

StagedMoveGenerator::StagedMoveGenerator(const GameState& Position, const SearchStackState* SS, SearchLocalState& Local,
    int DistanceFromRoot, bool Quiescence)
    : position(Position)
    , local(Local)
    , ss(SS)
    , distanceFromRoot(DistanceFromRoot)
    , quiescence(Quiescence)
{
    if (quiescence)
        stage = Stage::GEN_LOUD;
    else
        stage = Stage::TT_MOVE;
}

bool StagedMoveGenerator::Next(Move& move)
{
    moveSEE = std::nullopt;

    if (stage == Stage::TT_MOVE)
    {
        TTmove = GetHashMove(position.Board(), distanceFromRoot);
        stage = Stage::GEN_LOUD;

        if (MoveIsLegal(position.Board(), TTmove))
        {
            move = TTmove;
            return true;
        }
    }

    if (stage == Stage::GEN_LOUD)
    {
        QuiescenceMoves(position.Board(), loudMoves);
        OrderMoves(loudMoves);
        current = loudMoves.begin();
        stage = Stage::GIVE_GOOD_LOUD;
    }

    if (stage == Stage::GIVE_GOOD_LOUD)
    {
        if (current != loudMoves.end() && current->SEE >= 0)
        {
            move = current->move;
            moveSEE = current->SEE;
            ++current;
            return true;
        }

        if (quiescence)
            return false;

        stage = Stage::GIVE_KILLER_1;
    }

    if (stage == Stage::GIVE_KILLER_1)
    {
        Killer1 = ss->killers[0];
        stage = Stage::GIVE_KILLER_2;

        if (Killer1 != TTmove && MoveIsLegal(position.Board(), Killer1))
        {
            move = Killer1;
            return true;
        }
    }

    if (stage == Stage::GIVE_KILLER_2)
    {
        Killer2 = ss->killers[1];
        stage = Stage::GIVE_BAD_LOUD;

        if (Killer2 != TTmove && MoveIsLegal(position.Board(), Killer2))
        {
            move = Killer2;
            return true;
        }
    }

    if (stage == Stage::GIVE_BAD_LOUD)
    {
        if (current != loudMoves.end())
        {
            move = current->move;
            moveSEE = current->SEE;
            ++current;
            return true;
        }
        else
        {
            stage = Stage::GEN_QUIET;
        }
    }

    if (skipQuiets)
        return false;

    if (stage == Stage::GEN_QUIET)
    {
        QuietMoves(position.Board(), quietMoves);
        OrderMoves(quietMoves);
        current = quietMoves.begin();
        stage = Stage::GIVE_QUIET;
    }

    if (stage == Stage::GIVE_QUIET)
    {
        if (current != quietMoves.end())
        {
            move = current->move;
            moveSEE = current->SEE;
            ++current;
            return true;
        }
    }

    return false;
}

void StagedMoveGenerator::AdjustHistory(const Move& move, int positive_adjustment, int negative_adjustment) const
{
    local.history.Add(position, ss, move, positive_adjustment);

    for (auto const& m : quietMoves)
    {
        if (m.move == move)
            break;

        local.history.Add(position, ss, m.move, negative_adjustment);
    }
}

void StagedMoveGenerator::SkipQuiets()
{
    skipQuiets = true;
}

void selection_sort(ExtendedMoveList& v)
{
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        std::iter_swap(it, std::max_element(it, v.end()));
    }
}

constexpr int PieceValues[] = { 91, 532, 568, 715, 1279, 5000, 91, 532, 568, 715, 1279, 5000 };

uint64_t AttackersToSq(const BoardState& board, Square sq)
{
    uint64_t pawn_mask = (board.GetPieceBB(PAWN, WHITE) & PawnAttacks[BLACK][sq]);
    pawn_mask |= (board.GetPieceBB(PAWN, BLACK) & PawnAttacks[WHITE][sq]);

    uint64_t bishops = board.GetPieceBB<QUEEN>() | board.GetPieceBB<BISHOP>();
    uint64_t rooks = board.GetPieceBB<QUEEN>() | board.GetPieceBB<ROOK>();
    uint64_t occ = board.GetAllPieces();

    return (pawn_mask & board.GetPieceBB<PAWN>()) | (AttackBB<KNIGHT>(sq) & board.GetPieceBB<KNIGHT>())
        | (AttackBB<KING>(sq) & board.GetPieceBB<KING>()) | (AttackBB<BISHOP>(sq, occ) & bishops)
        | (AttackBB<ROOK>(sq, occ) & rooks);
}

uint64_t GetLeastValuableAttacker(const BoardState& board, uint64_t attackers, Pieces& capturing, Players side)
{
    for (int i = 0; i < 6; i++)
    {
        capturing = Piece(PieceTypes(i), side);
        uint64_t pieces = board.GetPieceBB(capturing) & attackers;
        if (pieces)
            return pieces & (~pieces + 1); // isolate LSB
    }
    return 0;
}

int see(const BoardState& board, Move move)
{
    Square from = move.GetFrom();
    Square to = move.GetTo();

    int scores[32] { 0 };
    int index = 0;

    auto capturing = board.GetSquare(from);
    auto captured = board.GetSquare(to);
    auto attacker = ColourOfPiece(capturing);

    uint64_t from_set = (1ull << from);
    uint64_t occ = board.GetAllPieces(), bishops = 0, rooks = 0;

    bishops = rooks = board.GetPieceBB<QUEEN>();
    bishops |= board.GetPieceBB<BISHOP>();
    rooks |= board.GetPieceBB<ROOK>();

    uint64_t attack_def = AttackersToSq(board, to);
    scores[index] = PieceValues[captured];

    do
    {
        index++;
        attacker = !attacker;
        scores[index] = PieceValues[capturing] - scores[index - 1];

        if (-scores[index - 1] < 0 && scores[index] < 0)
            break;

        attack_def ^= from_set;
        occ ^= from_set;

        attack_def |= occ & ((bishops & AttackBB<BISHOP>(to, occ)) | (rooks & AttackBB<ROOK>(to, occ)));
        from_set = GetLeastValuableAttacker(board, attack_def, capturing, Players(attacker));
    } while (from_set);
    while (--index)
    {
        scores[index - 1] = -(-scores[index - 1] > scores[index] ? -scores[index - 1] : scores[index]);
    }
    return scores[0];
}

int16_t StagedMoveGenerator::GetSEE(Move move) const
{
    if (moveSEE)
        return *moveSEE;
    else
        return see(position.Board(), move);
}

void StagedMoveGenerator::OrderMoves(ExtendedMoveList& moves)
{
    static constexpr int16_t SCORE_QUEEN_PROMOTION = 30000;
    static constexpr int16_t SCORE_CAPTURE = 20000;
    static constexpr int16_t SCORE_UNDER_PROMOTION = -1;

    for (size_t i = 0; i < moves.size(); i++)
    {
        // Hash move
        if (moves[i].move == TTmove)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Killers
        else if (moves[i].move == Killer1)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        else if (moves[i].move == Killer2)
        {
            moves.erase(moves.begin() + i);
            i--;
        }

        // Promotions
        else if (moves[i].move.IsPromotion())
        {
            if (moves[i].move.GetFlag() == QUEEN_PROMOTION || moves[i].move.GetFlag() == QUEEN_PROMOTION_CAPTURE)
            {
                moves[i].score = SCORE_QUEEN_PROMOTION;
                moves[i].SEE = PieceValues[QUEEN];
            }
            else
            {
                moves[i].score = SCORE_UNDER_PROMOTION;
            }
        }

        // Captures
        else if (moves[i].move.IsCapture())
        {
            int SEE = 0;

            if (moves[i].move.GetFlag() != EN_PASSANT)
            {
                SEE = see(position.Board(), moves[i].move);
            }

            moves[i].score = SCORE_CAPTURE + SEE;
            moves[i].SEE = SEE;
        }

        // Quiet
        else
        {
            int history = local.history.Get(position, ss, moves[i].move);
            moves[i].score = std::clamp<int>(history, std::numeric_limits<decltype(moves[i].score)>::min(),
                std::numeric_limits<decltype(moves[i].score)>::max());
        }
    }

    selection_sort(moves);
}

Move GetHashMoveMinDepth(const BoardState& board, int min_depth, int distanceFromRoot)
{
    auto entry = tTable.GetEntryMinDepth(board.GetZobristKey(), distanceFromRoot, min_depth, board.half_turn_count);

    if (entry.has_value())
    {
        return entry->move;
    }

    return Move::Uninitialized;
}

Move GetHashMove(const BoardState& board, int min_depth)
{
    auto entry = tTable.GetEntry(board.GetZobristKey(), min_depth, board.half_turn_count);

    if (entry.has_value())
    {
        return entry->move;
    }

    return Move::Uninitialized;
}