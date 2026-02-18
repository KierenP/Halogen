#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/movegen.h"
#include "network/accumulator.hpp"
#include "network/arch.hpp"
#include "network/threat_inputs.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

// A single threat, stored as the original attacker→victim description.
struct ThreatDelta
{
    PieceType atk_pt;
    Side atk_color;
    Square atk_sq;
    PieceType vic_pt;
    Side vic_color;
    Square vic_sq;
};

// Pre-computed indices into ft_threat_weight for both perspectives.
struct ThreatIndices
{
    uint32_t white_idx;
    uint32_t black_idx;
};

// Threat input accumulator. Stores threat feature contributions per side (WHITE/BLACK perspective).
// Incrementally updated via threat deltas computed in store_lazy_updates.
struct ThreatAccumulator
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, N_SIDES> side = {};

    bool operator==(const ThreatAccumulator& rhs) const
    {
        return side == rhs.side;
    }

    // Maximum number of threat deltas per move. A move changes at most 4 squares (castling),
    // each square can be involved in threats as attacker and victim, plus discovered attacks.
    static constexpr size_t MAX_THREAT_DELTAS = 256;

    // Threat deltas: features to add and subtract from the previous accumulator
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_adds = {};
    size_t n_threat_adds = 0;
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_subs = {};
    size_t n_threat_subs = 0;

    // Look up the threat feature indices for a given attacker→victim pair from both perspectives.
    static ThreatIndices get_threat_indices(
        PieceType atk_pt, Side atk_color, Square atk_sq, PieceType vic_pt, Side vic_color, Square vic_sq)
    {
        // White perspective: white is side 0, black is side 1, squares are not flipped
        int w_atk_side = (atk_color == WHITE) ? 0 : 1;
        int w_vic_side = (vic_color == WHITE) ? 0 : 1;
        int w_atk_idx = atk_pt * 2 + w_atk_side;
        int w_vic_idx = vic_pt * 2 + w_vic_side;
        uint32_t w_feat = NN::THREAT_TABLE.lookup[w_atk_idx][atk_sq][w_vic_idx][vic_sq];

        // Black perspective: black is side 0, white is side 1, squares are flipped vertically
        int b_atk_side = (atk_color == BLACK) ? 0 : 1;
        int b_vic_side = (vic_color == BLACK) ? 0 : 1;
        int b_atk_idx = atk_pt * 2 + b_atk_side;
        int b_vic_idx = vic_pt * 2 + b_vic_side;
        uint32_t b_feat = NN::THREAT_TABLE.lookup[b_atk_idx][atk_sq ^ 56][b_vic_idx][vic_sq ^ 56];

        assert(w_feat != NN::INVALID_THREAT);
        assert(b_feat != NN::INVALID_THREAT);

        return ThreatIndices { w_feat, b_feat };
    }

    // Get the attack bitboard for a piece type on a square with given occupancy,
    // masked to only include valid victim piece types for that attacker.
    static uint64_t get_threat_targets(PieceType atk_pt, Side atk_color, Square atk_sq, uint64_t occ, uint64_t pawns,
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
    static bool is_valid_victim_for(PieceType atk_pt, PieceType vic_pt)
    {
        return NN::can_threaten(atk_pt, vic_pt);
    }

    // Collect all threats where the piece on `sq` is the ATTACKER.
    template <typename Callback>
    static void collect_threats_from(const BoardState& board, Square sq, uint64_t occ, Callback&& cb)
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
            cb(ThreatDelta { pt, color, sq, enum_to<PieceType>(vic_piece), enum_to<Side>(vic_piece), vic_sq });
        }
    }

    // Collect all threats where the piece on `sq` is the VICTIM, excluding attackers matching exclude_mask.
    template <typename Callback>
    static void collect_threats_to(
        const BoardState& board, Square sq, uint64_t occ, uint64_t exclude_mask, Callback&& cb)
    {
        Piece piece_on_sq = board.get_square_piece(sq);
        PieceType vic_pt = enum_to<PieceType>(piece_on_sq);
        Side vic_color = enum_to<Side>(piece_on_sq);

        // Pawns attacking this square
        if (is_valid_victim_for(PAWN, vic_pt))
        {
            uint64_t w_pawn_attackers = PawnAttacks[BLACK][sq] & board.get_pieces_bb(WHITE_PAWN) & ~exclude_mask;
            while (w_pawn_attackers)
            {
                Square atk_sq = lsbpop(w_pawn_attackers);
                cb(ThreatDelta { PAWN, WHITE, atk_sq, vic_pt, vic_color, sq });
            }
            uint64_t b_pawn_attackers = PawnAttacks[WHITE][sq] & board.get_pieces_bb(BLACK_PAWN) & ~exclude_mask;
            while (b_pawn_attackers)
            {
                Square atk_sq = lsbpop(b_pawn_attackers);
                cb(ThreatDelta { PAWN, BLACK, atk_sq, vic_pt, vic_color, sq });
            }
        }

        // Knights
        if (is_valid_victim_for(KNIGHT, vic_pt))
        {
            uint64_t attackers = KnightAttacks[sq] & board.get_pieces_bb(KNIGHT) & ~exclude_mask;
            while (attackers)
            {
                Square atk_sq = lsbpop(attackers);
                cb(ThreatDelta {
                    KNIGHT, enum_to<Side>(board.get_square_piece(atk_sq)), atk_sq, vic_pt, vic_color, sq });
            }
        }

        // Bishops
        if (is_valid_victim_for(BISHOP, vic_pt))
        {
            uint64_t attackers = attack_bb<BISHOP>(sq, occ) & board.get_pieces_bb(BISHOP) & ~exclude_mask;
            while (attackers)
            {
                Square atk_sq = lsbpop(attackers);
                cb(ThreatDelta {
                    BISHOP, enum_to<Side>(board.get_square_piece(atk_sq)), atk_sq, vic_pt, vic_color, sq });
            }
        }

        // Rooks
        if (is_valid_victim_for(ROOK, vic_pt))
        {
            uint64_t attackers = attack_bb<ROOK>(sq, occ) & board.get_pieces_bb(ROOK) & ~exclude_mask;
            while (attackers)
            {
                Square atk_sq = lsbpop(attackers);
                cb(ThreatDelta { ROOK, enum_to<Side>(board.get_square_piece(atk_sq)), atk_sq, vic_pt, vic_color, sq });
            }
        }

        // Queens
        if (is_valid_victim_for(QUEEN, vic_pt))
        {
            uint64_t attackers = attack_bb<QUEEN>(sq, occ) & board.get_pieces_bb(QUEEN) & ~exclude_mask;
            while (attackers)
            {
                Square atk_sq = lsbpop(attackers);
                cb(ThreatDelta { QUEEN, enum_to<Side>(board.get_square_piece(atk_sq)), atk_sq, vic_pt, vic_color, sq });
            }
        }

        // Kings
        if (is_valid_victim_for(KING, vic_pt))
        {
            uint64_t attackers = KingAttacks[sq] & board.get_pieces_bb(KING) & ~exclude_mask;
            while (attackers)
            {
                Square atk_sq = lsbpop(attackers);
                cb(ThreatDelta { KING, enum_to<Side>(board.get_square_piece(atk_sq)), atk_sq, vic_pt, vic_color, sq });
            }
        }
    }

    // Collect threats from sliding pieces that pass through a given square.
    // These are threats between a slider and a victim where the ray passes through `through_sq`.
    //
    // For discoveries: slider_occ should have through_sq removed, victim_occ is the new occupancy.
    // For blocks: slider_occ should have through_sq removed, victim_occ is the old occupancy.
    template <typename Callback>
    static void collect_xray_threats_through(const BoardState& board, Square through_sq, uint64_t slider_occ,
        uint64_t victim_occ, uint64_t exclude_mask, Callback&& cb)
    {
        // Diagonal sliders (bishops and queens on diagonals)
        uint64_t diagonal_attackers = attack_bb<BISHOP>(through_sq, slider_occ) & ~exclude_mask;
        uint64_t bishops = diagonal_attackers & board.get_pieces_bb(BISHOP);
        uint64_t queens_diag = diagonal_attackers & board.get_pieces_bb(QUEEN);

        while (bishops)
        {
            Square slider_sq = lsbpop(bishops);
            Side slider_color = enum_to<Side>(board.get_square_piece(slider_sq));

            // Find squares reachable without blocker but not with blocker
            uint64_t attacks_open = attack_bb<BISHOP>(slider_sq, victim_occ);
            uint64_t attacks_blocked = attack_bb<BISHOP>(slider_sq, victim_occ | SquareBB[through_sq]);
            uint64_t beyond = (attacks_open & ~attacks_blocked) & victim_occ & ~exclude_mask;

            if (beyond)
            {
                Square vic_sq = lsbpop(beyond);
                Piece vic_piece = board.get_square_piece(vic_sq);
                PieceType vic_pt = enum_to<PieceType>(vic_piece);
                if (is_valid_victim_for(BISHOP, vic_pt))
                    cb(ThreatDelta { BISHOP, slider_color, slider_sq, vic_pt, enum_to<Side>(vic_piece), vic_sq });
            }
        }

        while (queens_diag)
        {
            Square slider_sq = lsbpop(queens_diag);
            Side slider_color = enum_to<Side>(board.get_square_piece(slider_sq));

            uint64_t attacks_open = attack_bb<BISHOP>(slider_sq, victim_occ);
            uint64_t attacks_blocked = attack_bb<BISHOP>(slider_sq, victim_occ | SquareBB[through_sq]);
            uint64_t beyond = (attacks_open & ~attacks_blocked) & victim_occ & ~exclude_mask;

            while (beyond)
            {
                Square vic_sq = lsbpop(beyond);
                Piece vic_piece = board.get_square_piece(vic_sq);
                PieceType vic_pt = enum_to<PieceType>(vic_piece);
                if (is_valid_victim_for(QUEEN, vic_pt))
                    cb(ThreatDelta { QUEEN, slider_color, slider_sq, vic_pt, enum_to<Side>(vic_piece), vic_sq });
            }
        }

        // Orthogonal sliders (rooks and queens on ranks/files)
        uint64_t orthogonal_attackers = attack_bb<ROOK>(through_sq, slider_occ) & ~exclude_mask;
        uint64_t rooks = orthogonal_attackers & board.get_pieces_bb(ROOK);
        uint64_t queens_orth = orthogonal_attackers & board.get_pieces_bb(QUEEN);

        while (rooks)
        {
            Square slider_sq = lsbpop(rooks);
            Side slider_color = enum_to<Side>(board.get_square_piece(slider_sq));

            uint64_t attacks_open = attack_bb<ROOK>(slider_sq, victim_occ);
            uint64_t attacks_blocked = attack_bb<ROOK>(slider_sq, victim_occ | SquareBB[through_sq]);
            uint64_t beyond = (attacks_open & ~attacks_blocked) & victim_occ & ~exclude_mask;

            while (beyond)
            {
                Square vic_sq = lsbpop(beyond);
                Piece vic_piece = board.get_square_piece(vic_sq);
                PieceType vic_pt = enum_to<PieceType>(vic_piece);
                if (is_valid_victim_for(ROOK, vic_pt))
                    cb(ThreatDelta { ROOK, slider_color, slider_sq, vic_pt, enum_to<Side>(vic_piece), vic_sq });
            }
        }

        while (queens_orth)
        {
            Square slider_sq = lsbpop(queens_orth);
            Side slider_color = enum_to<Side>(board.get_square_piece(slider_sq));

            uint64_t attacks_open = attack_bb<ROOK>(slider_sq, victim_occ);
            uint64_t attacks_blocked = attack_bb<ROOK>(slider_sq, victim_occ | SquareBB[through_sq]);
            uint64_t beyond = (attacks_open & ~attacks_blocked) & victim_occ & ~exclude_mask;

            while (beyond)
            {
                Square vic_sq = lsbpop(beyond);
                Piece vic_piece = board.get_square_piece(vic_sq);
                PieceType vic_pt = enum_to<PieceType>(vic_piece);
                if (is_valid_victim_for(QUEEN, vic_pt))
                    cb(ThreatDelta { QUEEN, slider_color, slider_sq, vic_pt, enum_to<Side>(vic_piece), vic_sq });
            }
        }
    }

    // Compute threat deltas for a move.
    //
    // sub_sqs: squares that had pieces on the old board that are no longer there (or changed identity).
    //          We collect threats involving those pieces on the old board → threat_subs.
    // add_sqs: squares that have pieces on the new board that weren't there before (or changed identity).
    //          We collect threats involving those pieces on the new board → threat_adds.
    // vacated_sqs: squares that went from occupied to empty (for x-ray discovery on new board → adds).
    // newly_occupied_sqs: squares that went from empty to occupied (for x-ray blocking on old board → subs).
    //
    // To avoid double-counting threats between two changed squares:
    //   - collect_threats_from: gets threats where the piece on the square is the ATTACKER (always safe)
    //   - collect_threats_to: gets threats where the piece is the VICTIM, excluding attackers that are
    //     themselves in the changed set (they'll be counted via their own collect_threats_from)
    //   - x-ray: excludes changed squares as both sliders and victims
    void compute_threat_deltas(const BoardState& prev_board, const BoardState& post_board, const Square* sub_sqs,
        size_t n_sub_sqs, const Square* add_sqs, size_t n_add_sqs, const Square* vacated_sqs, size_t n_vacated,
        const Square* newly_occupied_sqs, size_t n_newly_occupied)
    {
        n_threat_adds = 0;
        n_threat_subs = 0;

        uint64_t old_occ = prev_board.get_pieces_bb();
        uint64_t new_occ = post_board.get_pieces_bb();

        // Build exclude masks once upfront
        uint64_t sub_mask = 0;
        for (size_t i = 0; i < n_sub_sqs; i++)
            sub_mask |= SquareBB[sub_sqs[i]];

        uint64_t add_mask = 0;
        for (size_t i = 0; i < n_add_sqs; i++)
            add_mask |= SquareBB[add_sqs[i]];

        uint64_t all_changed_mask = sub_mask | add_mask;

        // --- Remove old threats involving sub squares ---
        for (size_t i = 0; i < n_sub_sqs; i++)
        {
            Square sq = sub_sqs[i];

            // Threats FROM this piece as attacker
            collect_threats_from(prev_board, sq, old_occ,
                [&](ThreatDelta td)
                {
                    assert(n_threat_subs < MAX_THREAT_DELTAS);
                    threat_subs[n_threat_subs++] = td;
                });

            // Threats TO this piece as victim, excluding other sub squares as attackers
            collect_threats_to(prev_board, sq, old_occ, sub_mask,
                [&](ThreatDelta td)
                {
                    assert(n_threat_subs < MAX_THREAT_DELTAS);
                    threat_subs[n_threat_subs++] = td;
                });
        }

        // --- Add new threats involving add squares ---
        for (size_t i = 0; i < n_add_sqs; i++)
        {
            Square sq = add_sqs[i];

            // Threats FROM this piece as attacker
            collect_threats_from(post_board, sq, new_occ,
                [&](ThreatDelta td)
                {
                    assert(n_threat_adds < MAX_THREAT_DELTAS);
                    threat_adds[n_threat_adds++] = td;
                });

            // Threats TO this piece as victim, excluding other add squares as attackers
            collect_threats_to(post_board, sq, new_occ, add_mask,
                [&](ThreatDelta td)
                {
                    assert(n_threat_adds < MAX_THREAT_DELTAS);
                    threat_adds[n_threat_adds++] = td;
                });
        }

        // --- X-ray: discovered attacks through vacated squares ---
        for (size_t i = 0; i < n_vacated; i++)
        {
            Square sq = vacated_sqs[i];

            // slider_occ: old occupancy with only THIS vacated square removed.
            // This ensures only sliders that were actually blocked by THIS square are found.
            // When multiple squares are vacated (e.g. en passant), each vacated square only
            // gets credit for discoveries where it was the nearest blocker to the slider.
            uint64_t slider_occ = old_occ & ~SquareBB[sq];

            // victim_occ: real new occupancy, so we see the full discovery reach
            // (all vacated squares empty, all newly occupied squares present).
            collect_xray_threats_through(post_board, sq, slider_occ, new_occ, all_changed_mask,
                [&](ThreatDelta td)
                {
                    assert(n_threat_adds < MAX_THREAT_DELTAS);
                    threat_adds[n_threat_adds++] = td;
                });
        }

        // --- X-ray: blocked attacks through newly occupied squares ---
        for (size_t i = 0; i < n_newly_occupied; i++)
        {
            Square sq = newly_occupied_sqs[i];

            // slider_occ: new occupancy with only THIS newly-occupied square removed.
            // This ensures only sliders that are now blocked by THIS square are found.
            uint64_t slider_occ = new_occ & ~SquareBB[sq];

            // victim_occ: old occupancy, so we see what reach the slider had before blocking.
            collect_xray_threats_through(prev_board, sq, slider_occ, old_occ, all_changed_mask,
                [&](ThreatDelta td)
                {
                    assert(n_threat_subs < MAX_THREAT_DELTAS);
                    threat_subs[n_threat_subs++] = td;
                });
        }
    }

    // Compute all threats from scratch for a given board position (used at root / recalculation).
    void compute(const BoardState& board, const NN::network& net)
    {
        side = {};

        uint64_t occ = board.get_pieces_bb();

        const auto pawns = board.get_pieces_bb(PAWN);
        const auto knights = board.get_pieces_bb(KNIGHT);
        const auto bishops = board.get_pieces_bb(BISHOP);
        const auto rooks = board.get_pieces_bb(ROOK);
        const auto queens = board.get_pieces_bb(QUEEN);
        const auto kings = board.get_pieces_bb(KING);

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
                    PieceType vic_pt = enum_to<PieceType>(vic_piece);
                    Side vic_color = enum_to<Side>(vic_piece);

                    auto idx = get_threat_indices(atk_pt, atk_color, atk_sq, vic_pt, vic_color, vic_sq);

                    NN::add1(side[WHITE], net.ft_threat_weight[idx.white_idx]);
                    NN::add1(side[BLACK], net.ft_threat_weight[idx.black_idx]);
                }
            }
        }
    }

    // Apply stored threat deltas to produce the new threat accumulator from the previous one.
    void apply_deltas(const ThreatAccumulator& prev, const NN::network& net)
    {
        side = prev.side;

        // ~3% speedup from prefetching
        for (size_t i = 0; i < n_threat_subs; i++)
        {
            auto idx = get_threat_indices(threat_subs[i].atk_pt, threat_subs[i].atk_color, threat_subs[i].atk_sq,
                threat_subs[i].vic_pt, threat_subs[i].vic_color, threat_subs[i].vic_sq);
            __builtin_prefetch(&net.ft_threat_weight[idx.white_idx]);
            __builtin_prefetch(&net.ft_threat_weight[idx.black_idx]);
            NN::sub1(side[WHITE], net.ft_threat_weight[idx.white_idx]);
            NN::sub1(side[BLACK], net.ft_threat_weight[idx.black_idx]);
        }

        for (size_t i = 0; i < n_threat_adds; i++)
        {
            auto idx = get_threat_indices(threat_adds[i].atk_pt, threat_adds[i].atk_color, threat_adds[i].atk_sq,
                threat_adds[i].vic_pt, threat_adds[i].vic_color, threat_adds[i].vic_sq);
            __builtin_prefetch(&net.ft_threat_weight[idx.white_idx]);
            __builtin_prefetch(&net.ft_threat_weight[idx.black_idx]);
            NN::add1(side[WHITE], net.ft_threat_weight[idx.white_idx]);
            NN::add1(side[BLACK], net.ft_threat_weight[idx.black_idx]);
        }
    }
};
