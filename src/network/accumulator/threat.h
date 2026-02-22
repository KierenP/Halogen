#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/movegen.h"
#include "network/arch.hpp"
#include "network/inputs/threat.h"
#include "network/simd/accumulator.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace NN::Threats
{

// A single threat, stored as the original attacker->victim description. w_king and b_king are same for all
// ThreatDeltas, so they are stored in the accumulator
struct ThreatDelta
{
    Piece atk_pt;
    Square atk_sq;
    Piece vic_pt;
    Square vic_sq;
};

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

    // data for lazy updates
    bool white_threats_requires_recalculation = false;
    bool black_threats_requires_recalculation = false;
    bool acc_is_valid = false;

    // Threat deltas: features to add and subtract from the previous accumulator
    Square w_king;
    Square b_king;
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_adds = {};
    size_t n_threat_adds = 0;
    std::array<ThreatDelta, MAX_THREAT_DELTAS> threat_subs = {};
    size_t n_threat_subs = 0;

    // Look up the threat feature indices for a given attacker->victim pair from both perspectives.
    static ThreatIndices get_threat_indices(PieceType atk_pt, Side atk_color, Square atk_sq, PieceType vic_pt,
        Side vic_color, Square vic_sq, Square w_king, Square b_king)
    {
        int w_flip = enum_to<File>(w_king) <= FILE_D ? 0 : 7;
        int b_flip = enum_to<File>(b_king) <= FILE_D ? 0 : 7;

        // White perspective: white is side 0, black is side 1, squares are not flipped
        int w_atk_side = (atk_color == WHITE) ? 0 : 1;
        int w_vic_side = (vic_color == WHITE) ? 0 : 1;
        int w_atk_idx = atk_pt * 2 + w_atk_side;
        int w_vic_idx = vic_pt * 2 + w_vic_side;
        uint32_t w_feat = get_threat_index(w_atk_idx, atk_sq ^ w_flip, w_vic_idx, vic_sq ^ w_flip);

        // Black perspective: black is side 0, white is side 1, squares are flipped vertically
        int b_atk_side = (atk_color == BLACK) ? 0 : 1;
        int b_vic_side = (vic_color == BLACK) ? 0 : 1;
        int b_atk_idx = atk_pt * 2 + b_atk_side;
        int b_vic_idx = vic_pt * 2 + b_vic_side;
        uint32_t b_feat = get_threat_index(b_atk_idx, atk_sq ^ 56 ^ b_flip, b_vic_idx, vic_sq ^ 56 ^ b_flip);

        assert(w_feat != INVALID_THREAT);
        assert(b_feat != INVALID_THREAT);

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
        return can_threaten(atk_pt, vic_pt);
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
            cb(ThreatDelta { piece_on_sq, sq, vic_piece, vic_sq });
        }
    }

    // Collect all threats where the piece on `sq` is the VICTIM, excluding attackers matching exclude_mask.
    template <typename Callback>
    static void collect_threats_to(
        const BoardState& board, Square sq, uint64_t occ, uint64_t exclude_mask, Callback&& cb)
    {
        Piece piece_on_sq = board.get_square_piece(sq);
        PieceType vic_pt = enum_to<PieceType>(piece_on_sq);

        // Pawns attacking this square
        if (is_valid_victim_for(PAWN, vic_pt))
        {
            uint64_t w_pawn_attackers = PawnAttacks[BLACK][sq] & board.get_pieces_bb(WHITE_PAWN) & ~exclude_mask;
            while (w_pawn_attackers)
            {
                Square atk_sq = lsbpop(w_pawn_attackers);
                cb(ThreatDelta { WHITE_PAWN, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_pawn_attackers = PawnAttacks[WHITE][sq] & board.get_pieces_bb(BLACK_PAWN) & ~exclude_mask;
            while (b_pawn_attackers)
            {
                Square atk_sq = lsbpop(b_pawn_attackers);
                cb(ThreatDelta { BLACK_PAWN, atk_sq, piece_on_sq, sq });
            }
        }

        // Knights
        if (is_valid_victim_for(KNIGHT, vic_pt))
        {
            uint64_t w_attackers = KnightAttacks[sq] & board.get_pieces_bb(WHITE_KNIGHT) & ~exclude_mask;
            while (w_attackers)
            {
                Square atk_sq = lsbpop(w_attackers);
                cb(ThreatDelta { WHITE_KNIGHT, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_attackers = KnightAttacks[sq] & board.get_pieces_bb(BLACK_KNIGHT) & ~exclude_mask;
            while (b_attackers)
            {
                Square atk_sq = lsbpop(b_attackers);
                cb(ThreatDelta { BLACK_KNIGHT, atk_sq, piece_on_sq, sq });
            }
        }

        // Bishops
        if (is_valid_victim_for(BISHOP, vic_pt))
        {
            uint64_t w_attackers = attack_bb<BISHOP>(sq, occ) & board.get_pieces_bb(WHITE_BISHOP) & ~exclude_mask;
            while (w_attackers)
            {
                Square atk_sq = lsbpop(w_attackers);
                cb(ThreatDelta { WHITE_BISHOP, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_attackers = attack_bb<BISHOP>(sq, occ) & board.get_pieces_bb(BLACK_BISHOP) & ~exclude_mask;
            while (b_attackers)
            {
                Square atk_sq = lsbpop(b_attackers);
                cb(ThreatDelta { BLACK_BISHOP, atk_sq, piece_on_sq, sq });
            }
        }

        // Rooks
        if (is_valid_victim_for(ROOK, vic_pt))
        {
            uint64_t w_attackers = attack_bb<ROOK>(sq, occ) & board.get_pieces_bb(WHITE_ROOK) & ~exclude_mask;
            while (w_attackers)
            {
                Square atk_sq = lsbpop(w_attackers);
                cb(ThreatDelta { WHITE_ROOK, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_attackers = attack_bb<ROOK>(sq, occ) & board.get_pieces_bb(BLACK_ROOK) & ~exclude_mask;
            while (b_attackers)
            {
                Square atk_sq = lsbpop(b_attackers);
                cb(ThreatDelta { BLACK_ROOK, atk_sq, piece_on_sq, sq });
            }
        }

        // Queens
        if (is_valid_victim_for(QUEEN, vic_pt))
        {
            uint64_t w_attackers = attack_bb<QUEEN>(sq, occ) & board.get_pieces_bb(WHITE_QUEEN) & ~exclude_mask;
            while (w_attackers)
            {
                Square atk_sq = lsbpop(w_attackers);
                cb(ThreatDelta { WHITE_QUEEN, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_attackers = attack_bb<QUEEN>(sq, occ) & board.get_pieces_bb(BLACK_QUEEN) & ~exclude_mask;
            while (b_attackers)
            {
                Square atk_sq = lsbpop(b_attackers);
                cb(ThreatDelta { BLACK_QUEEN, atk_sq, piece_on_sq, sq });
            }
        }

        // Kings
        if (is_valid_victim_for(KING, vic_pt))
        {
            uint64_t w_attackers = KingAttacks[sq] & board.get_pieces_bb(WHITE_KING) & ~exclude_mask;
            while (w_attackers)
            {
                Square atk_sq = lsbpop(w_attackers);
                cb(ThreatDelta { WHITE_KING, atk_sq, piece_on_sq, sq });
            }
            uint64_t b_attackers = KingAttacks[sq] & board.get_pieces_bb(BLACK_KING) & ~exclude_mask;
            while (b_attackers)
            {
                Square atk_sq = lsbpop(b_attackers);
                cb(ThreatDelta { BLACK_KING, atk_sq, piece_on_sq, sq });
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
                    cb(ThreatDelta { get_piece(BISHOP, slider_color), slider_sq, vic_piece, vic_sq });
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
                    cb(ThreatDelta { get_piece(QUEEN, slider_color), slider_sq, vic_piece, vic_sq });
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
                    cb(ThreatDelta { get_piece(ROOK, slider_color), slider_sq, vic_piece, vic_sq });
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
                    cb(ThreatDelta { get_piece(QUEEN, slider_color), slider_sq, vic_piece, vic_sq });
            }
        }
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
    void store_lazy_updates(
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
        for (auto sub_copy = sub_bb; sub_copy != EMPTY;)
        {
            Square sq = lsbpop(sub_copy);

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
        for (auto add_copy = add_bb; add_copy != EMPTY;)
        {
            Square sq = lsbpop(add_copy);

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

        // --- X-ray: discovered attacks through vacated squares ---
        while (newly_vacated_bb)
        {
            Square sq = lsbpop(newly_vacated_bb);

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
        while (newly_occupied_bb)
        {
            Square sq = lsbpop(newly_occupied_bb);

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

    // Compute all threats from scratch for a given board position.
    void recalculate_from_scratch(const BoardState& board, const NN::network& net)
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
                    PieceType vic_pt = enum_to<PieceType>(vic_piece);
                    Side vic_color = enum_to<Side>(vic_piece);

                    auto idx = get_threat_indices(atk_pt, atk_color, atk_sq, vic_pt, vic_color, vic_sq, w_king, b_king);

                    NN::add1(side[WHITE], net.ft_threat_weight[idx.white_idx]);
                    NN::add1(side[BLACK], net.ft_threat_weight[idx.black_idx]);
                }
            }
        }

        acc_is_valid = true;
    }

    void recalculate_side_from_scratch(const BoardState& board, const NN::network& net, Side perspective)
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
                    PieceType vic_pt = enum_to<PieceType>(vic_piece);
                    Side vic_color = enum_to<Side>(vic_piece);

                    auto idx = get_threat_indices(atk_pt, atk_color, atk_sq, vic_pt, vic_color, vic_sq, w_king, b_king);
                    auto side_idx = (perspective == WHITE) ? idx.white_idx : idx.black_idx;
                    NN::add1(side[perspective], net.ft_threat_weight[side_idx]);
                }
            }
        }

        acc_is_valid = true;
    }

    // Apply stored threat deltas to produce the new threat accumulator from the previous one.
    void apply_lazy_updates(const ThreatAccumulator& prev, const BoardState& board, const NN::network& net)
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
                auto idx = get_threat_indices(enum_to<PieceType>(threat_subs[i].atk_pt),
                    enum_to<Side>(threat_subs[i].atk_pt), threat_subs[i].atk_sq,
                    enum_to<PieceType>(threat_subs[i].vic_pt), enum_to<Side>(threat_subs[i].vic_pt),
                    threat_subs[i].vic_sq, w_king, b_king);
                b_sub_delta_indicies[b_sub_delta_indicies_size++] = idx.black_idx;
            }

            for (size_t i = 0; i < n_threat_adds; i++)
            {
                auto idx = get_threat_indices(enum_to<PieceType>(threat_adds[i].atk_pt),
                    enum_to<Side>(threat_adds[i].atk_pt), threat_adds[i].atk_sq,
                    enum_to<PieceType>(threat_adds[i].vic_pt), enum_to<Side>(threat_adds[i].vic_pt),
                    threat_adds[i].vic_sq, w_king, b_king);
                b_add_delta_indicies[b_add_delta_indicies_size++] = idx.black_idx;
            }
        }
        else if (black_threats_requires_recalculation)
        {
            for (size_t i = 0; i < n_threat_subs; i++)
            {
                auto idx = get_threat_indices(enum_to<PieceType>(threat_subs[i].atk_pt),
                    enum_to<Side>(threat_subs[i].atk_pt), threat_subs[i].atk_sq,
                    enum_to<PieceType>(threat_subs[i].vic_pt), enum_to<Side>(threat_subs[i].vic_pt),
                    threat_subs[i].vic_sq, w_king, b_king);
                w_sub_delta_indicies[w_sub_delta_indicies_size++] = idx.white_idx;
            }

            for (size_t i = 0; i < n_threat_adds; i++)
            {
                auto idx = get_threat_indices(enum_to<PieceType>(threat_adds[i].atk_pt),
                    enum_to<Side>(threat_adds[i].atk_pt), threat_adds[i].atk_sq,
                    enum_to<PieceType>(threat_adds[i].vic_pt), enum_to<Side>(threat_adds[i].vic_pt),
                    threat_adds[i].vic_sq, w_king, b_king);
                w_add_delta_indicies[w_add_delta_indicies_size++] = idx.white_idx;
            }
        }
        else
        {
            for (size_t i = 0; i < n_threat_subs; i++)
            {
                auto idx = get_threat_indices(enum_to<PieceType>(threat_subs[i].atk_pt),
                    enum_to<Side>(threat_subs[i].atk_pt), threat_subs[i].atk_sq,
                    enum_to<PieceType>(threat_subs[i].vic_pt), enum_to<Side>(threat_subs[i].vic_pt),
                    threat_subs[i].vic_sq, w_king, b_king);
                w_sub_delta_indicies[w_sub_delta_indicies_size++] = idx.white_idx;
                b_sub_delta_indicies[b_sub_delta_indicies_size++] = idx.black_idx;
            }

            for (size_t i = 0; i < n_threat_adds; i++)
            {
                auto idx = get_threat_indices(enum_to<PieceType>(threat_adds[i].atk_pt),
                    enum_to<Side>(threat_adds[i].atk_pt), threat_adds[i].atk_sq,
                    enum_to<PieceType>(threat_adds[i].vic_pt), enum_to<Side>(threat_adds[i].vic_pt),
                    threat_adds[i].vic_sq, w_king, b_king);
                w_add_delta_indicies[w_add_delta_indicies_size++] = idx.white_idx;
                b_add_delta_indicies[b_add_delta_indicies_size++] = idx.black_idx;
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
};

}