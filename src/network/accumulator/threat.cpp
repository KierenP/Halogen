#include "threat.h"

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/movegen.h"
#include "network/inputs/threat.h"
#include "network/simd/accumulator.hpp"

#include <cassert>

#if defined(USE_SSE4)
#include <immintrin.h>
#endif

namespace NN::Threats
{

bool ThreatAccumulator::operator==(const ThreatAccumulator& rhs) const
{
    return side == rhs.side;
}

// Get the attack bitboard for a piece type on a square with given occupancy,
// masked to only include valid victim piece types for that attacker.
uint64_t get_threat_targets(PieceType atk_pt, Side atk_color, Square atk_sq, uint64_t occ, uint64_t pawns,
    uint64_t knights, uint64_t bishops, uint64_t rooks, [[maybe_unused]] uint64_t queens, uint64_t kings)
{
    switch (atk_pt)
    {
    case PAWN:
        return PawnAttacks[atk_color][atk_sq] & (pawns | knights | rooks);
    case KNIGHT:
        return KnightAttacks[atk_sq] & occ;
    case BISHOP:
        return attack_bb<BISHOP>(atk_sq, occ) & (pawns | knights | bishops | rooks | kings);
    case ROOK:
        return attack_bb<ROOK>(atk_sq, occ) & (pawns | knights | bishops | rooks | kings);
    case QUEEN:
        return attack_bb<QUEEN>(atk_sq, occ) & occ;
    case KING:
        return KingAttacks[atk_sq] & (pawns | knights | bishops | rooks);
    default:
        return 0;
    }
}

// Check if atk_pt can threaten vic_pt (used when iterating potential attackers to a square)
bool is_valid_victim_for(PieceType atk_pt, PieceType vic_pt)
{
    return can_threaten(atk_pt, vic_pt);
}

// Collect all threats where the piece on `sq` is the ATTACKER.
template <typename Callback>
void collect_threats_from(const BoardState& board, Square sq, uint64_t occ, Callback&& cb)
{
    const auto pawns = board.get_pieces_bb(PAWN);
    const auto knights = board.get_pieces_bb(KNIGHT);
    const auto bishops = board.get_pieces_bb(BISHOP);
    const auto rooks = board.get_pieces_bb(ROOK);
    const auto queens = board.get_pieces_bb(QUEEN);
    const auto kings = board.get_pieces_bb(KING);

    Piece piece_on_sq = board.get_square_piece(sq);
    PieceType pt = enum_to<PieceType>(piece_on_sq);
    Side color = enum_to<Side>(piece_on_sq);

    uint64_t targets = get_threat_targets(pt, color, sq, occ, pawns, knights, bishops, rooks, queens, kings);
    while (targets)
    {
        Square vic_sq = lsbpop(targets);
        Piece vic_piece = board.get_square_piece(vic_sq);
        cb(ThreatDelta { piece_on_sq, sq, vic_piece, vic_sq });
    }
}

// Collect all threats where the piece on `sq` is the VICTIM, excluding attackers matching exclude_mask.
template <typename Callback>
void collect_threats_to(const BoardState& board, Square sq, uint64_t occ, uint64_t exclude_mask, Callback&& cb)
{
    Piece piece_on_sq = board.get_square_piece(sq);
    PieceType vic_pt = enum_to<PieceType>(piece_on_sq);
    const uint64_t not_excluded = ~exclude_mask;

    // ============================================================
    // Helper: emit direct attackers
    // ============================================================

    auto emit_direct = [&]<Piece piece>(uint64_t attackers)
    {
        attackers &= not_excluded;
        while (attackers)
        {
            Square from = lsbpop(attackers);
            cb(ThreatDelta { piece, from, piece_on_sq, sq });
        }
    };

    // --- Pawns
    if (is_valid_victim_for(PAWN, vic_pt))
    {
        emit_direct.template operator()<WHITE_PAWN>(PawnAttacks[BLACK][sq] & board.get_pieces_bb(WHITE_PAWN));
        emit_direct.template operator()<BLACK_PAWN>(PawnAttacks[WHITE][sq] & board.get_pieces_bb(BLACK_PAWN));
    }

    // --- Knights
    if (is_valid_victim_for(KNIGHT, vic_pt))
    {
        const uint64_t knight_attacks = KnightAttacks[sq];
        emit_direct.template operator()<WHITE_KNIGHT>(knight_attacks & board.get_pieces_bb(WHITE_KNIGHT));
        emit_direct.template operator()<BLACK_KNIGHT>(knight_attacks & board.get_pieces_bb(BLACK_KNIGHT));
    }

    // --- Kings
    if (is_valid_victim_for(KING, vic_pt))
    {
        const uint64_t king_attacks = KingAttacks[sq];
        emit_direct.template operator()<WHITE_KING>(king_attacks & board.get_pieces_bb(WHITE_KING));
        emit_direct.template operator()<BLACK_KING>(king_attacks & board.get_pieces_bb(BLACK_KING));
    }

    // --- Sliding direct attacks (compute once)
    const uint64_t bishop_attacks = attack_bb<BISHOP>(sq, occ);
    const uint64_t rook_attacks = attack_bb<ROOK>(sq, occ);

    if (is_valid_victim_for(BISHOP, vic_pt))
    {
        emit_direct.template operator()<WHITE_BISHOP>(bishop_attacks & board.get_pieces_bb(WHITE_BISHOP));
        emit_direct.template operator()<BLACK_BISHOP>(bishop_attacks & board.get_pieces_bb(BLACK_BISHOP));
    }

    if (is_valid_victim_for(ROOK, vic_pt))
    {
        emit_direct.template operator()<WHITE_ROOK>(rook_attacks & board.get_pieces_bb(WHITE_ROOK));
        emit_direct.template operator()<BLACK_ROOK>(rook_attacks & board.get_pieces_bb(BLACK_ROOK));
    }

    if (is_valid_victim_for(QUEEN, vic_pt))
    {
        const uint64_t queen_attacks = bishop_attacks | rook_attacks;
        emit_direct.template operator()<WHITE_QUEEN>(queen_attacks & board.get_pieces_bb(WHITE_QUEEN));
        emit_direct.template operator()<BLACK_QUEEN>(queen_attacks & board.get_pieces_bb(BLACK_QUEEN));
    }
}

// for an added piece,
//  add_board == post_board,
//  sub_board == prev_board,
//  add_cb adds threats to ThreatAccumulator,
//  sub_cb subs threats to ThreatAccumulator
// for a removed piece,
//  add_board == prev_board,
//  sub_board == post_board,
//  add_cb subs threats to ThreatAccumulator,
//  sub_cb adds threats to ThreatAccumulator
template <typename AddCB, typename SubCB>
void collect_threats_to_plus_xray(const BoardState& add_board, const BoardState& sub_board, Square sq, uint64_t add_occ,
    uint64_t sub_occ, uint64_t exclude_mask, AddCB&& add_cb, SubCB&& sub_cb)
{
    const Piece victim = add_board.get_square_piece(sq);
    const PieceType vic_pt = enum_to<PieceType>(victim);
    const uint64_t not_excluded = ~exclude_mask;

    // ============================================================
    // Helper: emit direct attackers
    // ============================================================

    auto emit_direct = [&]<Piece piece>(uint64_t attackers)
    {
        attackers &= not_excluded;
        while (attackers)
        {
            Square from = lsbpop(attackers);
            add_cb(ThreatDelta { piece, from, victim, sq });
        }
    };

    // ============================================================
    // 1. Direct threats
    // ============================================================

    // --- Pawns
    if (is_valid_victim_for(PAWN, vic_pt))
    {
        emit_direct.template operator()<WHITE_PAWN>(PawnAttacks[BLACK][sq] & add_board.get_pieces_bb(WHITE_PAWN));
        emit_direct.template operator()<BLACK_PAWN>(PawnAttacks[WHITE][sq] & add_board.get_pieces_bb(BLACK_PAWN));
    }

    // --- Knights
    if (is_valid_victim_for(KNIGHT, vic_pt))
    {
        const uint64_t knight_attacks = KnightAttacks[sq];
        emit_direct.template operator()<WHITE_KNIGHT>(knight_attacks & add_board.get_pieces_bb(WHITE_KNIGHT));
        emit_direct.template operator()<BLACK_KNIGHT>(knight_attacks & add_board.get_pieces_bb(BLACK_KNIGHT));
    }

    // --- Kings
    if (is_valid_victim_for(KING, vic_pt))
    {
        const uint64_t king_attacks = KingAttacks[sq];
        emit_direct.template operator()<WHITE_KING>(king_attacks & add_board.get_pieces_bb(WHITE_KING));
        emit_direct.template operator()<BLACK_KING>(king_attacks & add_board.get_pieces_bb(BLACK_KING));
    }

    // --- Sliding direct attacks (compute once)
    const uint64_t bishop_attacks = attack_bb<BISHOP>(sq, add_occ);
    const uint64_t rook_attacks = attack_bb<ROOK>(sq, add_occ);

    if (is_valid_victim_for(BISHOP, vic_pt))
    {
        emit_direct.template operator()<WHITE_BISHOP>(bishop_attacks & add_board.get_pieces_bb(WHITE_BISHOP));
        emit_direct.template operator()<BLACK_BISHOP>(bishop_attacks & add_board.get_pieces_bb(BLACK_BISHOP));
    }

    if (is_valid_victim_for(ROOK, vic_pt))
    {
        emit_direct.template operator()<WHITE_ROOK>(rook_attacks & add_board.get_pieces_bb(WHITE_ROOK));
        emit_direct.template operator()<BLACK_ROOK>(rook_attacks & add_board.get_pieces_bb(BLACK_ROOK));
    }

    if (is_valid_victim_for(QUEEN, vic_pt))
    {
        const uint64_t queen_attacks = bishop_attacks | rook_attacks;
        emit_direct.template operator()<WHITE_QUEEN>(queen_attacks & add_board.get_pieces_bb(WHITE_QUEEN));
        emit_direct.template operator()<BLACK_QUEEN>(queen_attacks & add_board.get_pieces_bb(BLACK_QUEEN));
    }

    // ============================================================
    // 2. Unified X-Ray Sliders
    // ============================================================

    auto collect_xray = [&]<PieceType attack_tag, PieceType pt>(uint64_t candidate_sliders)
    {
        while (candidate_sliders)
        {
            Square slider_sq = lsbpop(candidate_sliders);
            Piece slider_piece = sub_board.get_square_piece(slider_sq);
            Side slider_side = enum_to<Side>(slider_piece);

            uint64_t attacks_open = attack_bb<attack_tag>(slider_sq, sub_occ);
            uint64_t attacks_blocked = attack_bb<attack_tag>(slider_sq, sub_occ | SquareBB[sq]);
            uint64_t beyond = (attacks_open & ~attacks_blocked) & sub_occ & not_excluded;

            while (beyond)
            {
                Square victim_sq = lsbpop(beyond);
                Piece xray_victim = sub_board.get_square_piece(victim_sq);
                PieceType xray_vpt = enum_to<PieceType>(xray_victim);

                if (is_valid_victim_for(pt, xray_vpt))
                {
                    sub_cb(ThreatDelta { get_piece(pt, slider_side), slider_sq, xray_victim, victim_sq });
                }
            }
        }
    };

    // Diagonal geometry -> bishop movement
    collect_xray.template operator()<BISHOP, BISHOP>(bishop_attacks & sub_board.get_pieces_bb(BISHOP) & not_excluded);
    collect_xray.template operator()<BISHOP, QUEEN>(bishop_attacks & sub_board.get_pieces_bb(QUEEN) & not_excluded);

    // Orthogonal geometry -> rook movement
    collect_xray.template operator()<ROOK, ROOK>(rook_attacks & sub_board.get_pieces_bb(ROOK) & not_excluded);
    collect_xray.template operator()<ROOK, QUEEN>(rook_attacks & sub_board.get_pieces_bb(QUEEN) & not_excluded);
}

// Compute threat deltas for a move.
//
// sub_sqs: squares that had pieces on the old board that are no longer there (or changed identity).
//          We collect threats involving those pieces on the old board -> threat_subs.
// add_sqs: squares that have pieces on the new board that weren't there before (or changed identity).
//          We collect threats involving those pieces on the new board -> threat_adds.
// vacated_sqs: squares that went from occupied to empty (for x-ray discovery on new board -> adds).
// newly_occupied_sqs: squares that went from empty to occupied (for x-ray blocking on old board -> subs).
//
// To avoid double-counting threats between two changed squares:
//   - collect_threats_from: gets threats where the piece on the square is the ATTACKER (always safe)
//   - collect_threats_to: gets threats where the piece is the VICTIM, excluding attackers that are
//     themselves in the changed set (they'll be counted via their own collect_threats_from)
//   - x-ray: excludes changed squares as both sliders and victims
void ThreatAccumulator::store_lazy_updates(
    const BoardState& prev_board, const BoardState& post_board, uint64_t sub_bb, uint64_t add_bb)
{
    n_threat_adds = 0;
    n_threat_subs = 0;

    uint64_t old_occ = prev_board.get_pieces_bb();
    uint64_t new_occ = post_board.get_pieces_bb();
    uint64_t all_changed_mask = sub_bb | add_bb;
    uint64_t newly_vacated_bb = sub_bb & ~add_bb;
    uint64_t newly_occupied_bb = add_bb & ~sub_bb;

    w_king = post_board.get_king_sq(WHITE);
    b_king = post_board.get_king_sq(BLACK);

    // --- Remove old threats involving sub squares ---
    for (auto copy = sub_bb & ~newly_vacated_bb; copy != EMPTY;)
    {
        Square sq = lsbpop(copy);

        // Threats FROM this piece as attacker
        collect_threats_from(prev_board, sq, old_occ,
            [&](ThreatDelta td)
            {
                assert(n_threat_subs < MAX_THREAT_DELTAS);
                threat_subs[n_threat_subs++] = td;
            });

        // Threats TO this piece as victim, excluding other sub squares as attackers
        collect_threats_to(prev_board, sq, old_occ, sub_bb,
            [&](ThreatDelta td)
            {
                assert(n_threat_subs < MAX_THREAT_DELTAS);
                threat_subs[n_threat_subs++] = td;
            });
    }

    // --- Add new threats involving add squares ---
    for (auto copy = add_bb & ~newly_occupied_bb; copy != EMPTY;)
    {
        Square sq = lsbpop(copy);

        // Threats FROM this piece as attacker
        collect_threats_from(post_board, sq, new_occ,
            [&](ThreatDelta td)
            {
                assert(n_threat_adds < MAX_THREAT_DELTAS);
                threat_adds[n_threat_adds++] = td;
            });

        // Threats TO this piece as victim, excluding other add squares as attackers
        collect_threats_to(post_board, sq, new_occ, add_bb,
            [&](ThreatDelta td)
            {
                assert(n_threat_adds < MAX_THREAT_DELTAS);
                threat_adds[n_threat_adds++] = td;
            });
    }

    for (auto copy = sub_bb & newly_vacated_bb; copy != EMPTY;)
    {
        Square sq = lsbpop(copy);

        // Threats FROM this piece as attacker
        collect_threats_from(prev_board, sq, old_occ,
            [&](ThreatDelta td)
            {
                assert(n_threat_subs < MAX_THREAT_DELTAS);
                threat_subs[n_threat_subs++] = td;
            });

        // Remove threats to this piece, add discovered xray threats through this piece
        collect_threats_to_plus_xray(
            prev_board, post_board, sq, old_occ, new_occ, all_changed_mask,
            [&](ThreatDelta td)
            {
                assert(n_threat_subs < MAX_THREAT_DELTAS);
                threat_subs[n_threat_subs++] = td;
            },
            [&](ThreatDelta td)
            {
                assert(n_threat_adds < MAX_THREAT_DELTAS);
                threat_adds[n_threat_adds++] = td;
            });
    }

    // --- Add new threats involving add squares ---
    for (auto copy = add_bb & newly_occupied_bb; copy != EMPTY;)
    {
        Square sq = lsbpop(copy);

        // Threats FROM this piece as attacker
        collect_threats_from(post_board, sq, new_occ,
            [&](ThreatDelta td)
            {
                assert(n_threat_adds < MAX_THREAT_DELTAS);
                threat_adds[n_threat_adds++] = td;
            });

        // Add threats to this piece, remove xray threats that are now blocked by this piece
        collect_threats_to_plus_xray(
            post_board, prev_board, sq, new_occ, old_occ, all_changed_mask,
            [&](ThreatDelta td)
            {
                assert(n_threat_adds < MAX_THREAT_DELTAS);
                threat_adds[n_threat_adds++] = td;
            },
            [&](ThreatDelta td)
            {
                assert(n_threat_subs < MAX_THREAT_DELTAS);
                threat_subs[n_threat_subs++] = td;
            });
    }
}

// Compute all threats from scratch for a given board position.
void ThreatAccumulator::recalculate_from_scratch(const BoardState& board, const NN::network& net)
{
    *this = {};

    uint64_t occ = board.get_pieces_bb();

    const auto pawns = board.get_pieces_bb(PAWN);
    const auto knights = board.get_pieces_bb(KNIGHT);
    const auto bishops = board.get_pieces_bb(BISHOP);
    const auto rooks = board.get_pieces_bb(ROOK);
    const auto queens = board.get_pieces_bb(QUEEN);
    const auto kings = board.get_pieces_bb(KING);

    w_king = board.get_king_sq(WHITE);
    b_king = board.get_king_sq(BLACK);

    for (int piece_idx = 0; piece_idx < N_PIECES; piece_idx++)
    {
        Piece atk_piece = static_cast<Piece>(piece_idx);
        PieceType atk_pt = enum_to<PieceType>(atk_piece);
        Side atk_color = enum_to<Side>(atk_piece);

        uint64_t atk_bb = board.get_pieces_bb(atk_piece);
        while (atk_bb)
        {
            Square atk_sq = lsbpop(atk_bb);

            uint64_t attacked
                = get_threat_targets(atk_pt, atk_color, atk_sq, occ, pawns, knights, bishops, rooks, queens, kings);

            while (attacked)
            {
                Square vic_sq = lsbpop(attacked);
                Piece vic_piece = board.get_square_piece(vic_sq);

                auto white_idx = get_threat_index<WHITE>(atk_piece, atk_sq, vic_piece, vic_sq, w_king);
                auto black_idx = get_threat_index<BLACK>(atk_piece, atk_sq, vic_piece, vic_sq, b_king);

                NN::add1(side[WHITE], net.ft_threat_weight[white_idx]);
                NN::add1(side[BLACK], net.ft_threat_weight[black_idx]);
            }
        }
    }

    acc_is_valid = true;
}

void ThreatAccumulator::recalculate_side_from_scratch(const BoardState& board, const NN::network& net, Side perspective)
{
    side[perspective] = {};

    uint64_t occ = board.get_pieces_bb();

    const auto pawns = board.get_pieces_bb(PAWN);
    const auto knights = board.get_pieces_bb(KNIGHT);
    const auto bishops = board.get_pieces_bb(BISHOP);
    const auto rooks = board.get_pieces_bb(ROOK);
    const auto queens = board.get_pieces_bb(QUEEN);
    const auto kings = board.get_pieces_bb(KING);

    w_king = board.get_king_sq(WHITE);
    b_king = board.get_king_sq(BLACK);

    for (int piece_idx = 0; piece_idx < N_PIECES; piece_idx++)
    {
        Piece atk_piece = static_cast<Piece>(piece_idx);
        PieceType atk_pt = enum_to<PieceType>(atk_piece);
        Side atk_color = enum_to<Side>(atk_piece);

        uint64_t atk_bb = board.get_pieces_bb(atk_piece);
        while (atk_bb)
        {
            Square atk_sq = lsbpop(atk_bb);

            uint64_t attacked
                = get_threat_targets(atk_pt, atk_color, atk_sq, occ, pawns, knights, bishops, rooks, queens, kings);

            while (attacked)
            {
                Square vic_sq = lsbpop(attacked);
                Piece vic_piece = board.get_square_piece(vic_sq);

                auto side_idx = (perspective == WHITE)
                    ? get_threat_index<WHITE>(atk_piece, atk_sq, vic_piece, vic_sq, w_king)
                    : get_threat_index<BLACK>(atk_piece, atk_sq, vic_piece, vic_sq, b_king);
                NN::add1(side[perspective], net.ft_threat_weight[side_idx]);
            }
        }
    }

    acc_is_valid = true;
}

// Apply stored threat deltas to produce the new threat accumulator from the previous one.
void ThreatAccumulator::apply_lazy_updates(
    const ThreatAccumulator& prev, const BoardState& board, const NN::network& net)
{
    // ~3% speedup from prefetching

    std::array<uint32_t, MAX_THREAT_DELTAS> w_add_delta_indicies;
    std::array<uint32_t, MAX_THREAT_DELTAS> w_sub_delta_indicies;
    std::array<uint32_t, MAX_THREAT_DELTAS> b_add_delta_indicies;
    std::array<uint32_t, MAX_THREAT_DELTAS> b_sub_delta_indicies;
    size_t w_add_delta_indicies_size = 0;
    size_t w_sub_delta_indicies_size = 0;
    size_t b_add_delta_indicies_size = 0;
    size_t b_sub_delta_indicies_size = 0;

    if (white_threats_requires_recalculation)
    {
        for (size_t i = 0; i < n_threat_subs; i++)
        {
            auto idx = get_threat_index<BLACK>(
                threat_subs[i].atk_pt, threat_subs[i].atk_sq, threat_subs[i].vic_pt, threat_subs[i].vic_sq, b_king);
            b_sub_delta_indicies[b_sub_delta_indicies_size++] = idx;
        }

        for (size_t i = 0; i < n_threat_adds; i++)
        {
            auto idx = get_threat_index<BLACK>(
                threat_adds[i].atk_pt, threat_adds[i].atk_sq, threat_adds[i].vic_pt, threat_adds[i].vic_sq, b_king);
            b_add_delta_indicies[b_add_delta_indicies_size++] = idx;
        }
    }
    else if (black_threats_requires_recalculation)
    {
        for (size_t i = 0; i < n_threat_subs; i++)
        {
            auto idx = get_threat_index<WHITE>(
                threat_subs[i].atk_pt, threat_subs[i].atk_sq, threat_subs[i].vic_pt, threat_subs[i].vic_sq, w_king);
            w_sub_delta_indicies[w_sub_delta_indicies_size++] = idx;
        }

        for (size_t i = 0; i < n_threat_adds; i++)
        {
            auto idx = get_threat_index<WHITE>(
                threat_adds[i].atk_pt, threat_adds[i].atk_sq, threat_adds[i].vic_pt, threat_adds[i].vic_sq, w_king);
            w_add_delta_indicies[w_add_delta_indicies_size++] = idx;
        }
    }
    else
    {
        for (size_t i = 0; i < n_threat_subs; i++)
        {
            auto w_idx = get_threat_index<WHITE>(
                threat_subs[i].atk_pt, threat_subs[i].atk_sq, threat_subs[i].vic_pt, threat_subs[i].vic_sq, w_king);
            auto b_idx = get_threat_index<BLACK>(
                threat_subs[i].atk_pt, threat_subs[i].atk_sq, threat_subs[i].vic_pt, threat_subs[i].vic_sq, b_king);
            w_sub_delta_indicies[w_sub_delta_indicies_size++] = w_idx;
            b_sub_delta_indicies[b_sub_delta_indicies_size++] = b_idx;
        }

        for (size_t i = 0; i < n_threat_adds; i++)
        {
            auto w_idx = get_threat_index<WHITE>(
                threat_adds[i].atk_pt, threat_adds[i].atk_sq, threat_adds[i].vic_pt, threat_adds[i].vic_sq, w_king);
            auto b_idx = get_threat_index<BLACK>(
                threat_adds[i].atk_pt, threat_adds[i].atk_sq, threat_adds[i].vic_pt, threat_adds[i].vic_sq, b_king);
            w_add_delta_indicies[w_add_delta_indicies_size++] = w_idx;
            b_add_delta_indicies[b_add_delta_indicies_size++] = b_idx;
        }
    }

    if (white_threats_requires_recalculation)
    {
        recalculate_side_from_scratch(board, net, WHITE);
        side[BLACK] = prev.side[BLACK];
        assert(w_sub_delta_indicies_size == 0 && w_add_delta_indicies_size == 0);
    }
    else if (black_threats_requires_recalculation)
    {
        recalculate_side_from_scratch(board, net, BLACK);
        side[WHITE] = prev.side[WHITE];
        assert(b_sub_delta_indicies_size == 0 && b_add_delta_indicies_size == 0);
    }
    else
    {
        side = prev.side;
    }

    size_t w_add_idx = 0;
    size_t w_sub_idx = 0;
    size_t b_add_idx = 0;
    size_t b_sub_idx = 0;

    while (w_sub_idx < w_sub_delta_indicies_size && w_add_idx < w_add_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[w_add_delta_indicies[w_add_idx]]);
        __builtin_prefetch(&net.ft_threat_weight[w_sub_delta_indicies[w_sub_idx]]);
        NN::add1sub1(side[WHITE], side[WHITE], net.ft_threat_weight[w_add_delta_indicies[w_add_idx++]],
            net.ft_threat_weight[w_sub_delta_indicies[w_sub_idx++]]);
    }

    while (w_sub_idx < w_sub_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[w_sub_delta_indicies[w_sub_idx]]);
        NN::sub1(side[WHITE], net.ft_threat_weight[w_sub_delta_indicies[w_sub_idx++]]);
    }

    while (w_add_idx < w_add_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[w_add_delta_indicies[w_add_idx]]);
        NN::add1(side[WHITE], net.ft_threat_weight[w_add_delta_indicies[w_add_idx++]]);
    }

    while (b_sub_idx < b_sub_delta_indicies_size && b_add_idx < b_add_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[b_add_delta_indicies[b_add_idx]]);
        __builtin_prefetch(&net.ft_threat_weight[b_sub_delta_indicies[b_sub_idx]]);
        NN::add1sub1(side[BLACK], side[BLACK], net.ft_threat_weight[b_add_delta_indicies[b_add_idx++]],
            net.ft_threat_weight[b_sub_delta_indicies[b_sub_idx++]]);
    }

    while (b_sub_idx < b_sub_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[b_sub_delta_indicies[b_sub_idx]]);
        NN::sub1(side[BLACK], net.ft_threat_weight[b_sub_delta_indicies[b_sub_idx++]]);
    }

    while (b_add_idx < b_add_delta_indicies_size)
    {
        __builtin_prefetch(&net.ft_threat_weight[b_add_delta_indicies[b_add_idx]]);
        NN::add1(side[BLACK], net.ft_threat_weight[b_add_delta_indicies[b_add_idx++]]);
    }

    acc_is_valid = true;
}

}