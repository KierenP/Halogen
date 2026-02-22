#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace NN::Threats
{

constexpr uint32_t INVALID_THREAT = UINT32_MAX;

// ============================================================
// Threat table construction
// ============================================================
//
// Threat inputs are (attacker_piece, attacker_sq, victim_piece, victim_sq) features that encode
// which pieces are attacking which. Deduplication removes symmetric threats to reduce the total
// count. See threat_inputs.md for full specification.
//
// The tables map each valid threat to a unique feature index. Invalid/duplicate threats map to
// INVALID_THREAT.

// Threat restrictions:
// - Pawn only threatens pawns, knights, rooks (6 victims)
// - Bishop/rook don't threaten queens; only queens threaten bishops/rooks
// - King only threatens pawns, knights, bishops, rooks (8 victims)
//
// Threat counts (no deduplication of symmetric same-piece threats):
// - pawn:   84 attacks * 6 victims  = 504
// - knight: 336 attacks * 12 victims = 4,032
// - bishop: 560 attacks * 10 victims = 5,600
// - rook:   896 attacks * 10 victims = 8,960
// - queen:  1456 attacks * 12 victims = 17,472
// - king:   420 attacks * 8 victims  = 3,360
// - TOTAL: (504 + 4032 + 5600 + 8960 + 17472 + 3360) * 2 sides = 79,856
//
// Check if an attacker piece type is allowed to threaten a victim piece type.
constexpr bool can_threaten(PieceType atk_piece, PieceType vic_piece)
{
    switch (atk_piece)
    {
    case PAWN:
        return vic_piece == PAWN || vic_piece == KNIGHT || vic_piece == ROOK;
    case BISHOP:
    case ROOK:
        return vic_piece != QUEEN;
    case KING:
        return vic_piece == PAWN || vic_piece == KNIGHT || vic_piece == BISHOP || vic_piece == ROOK;
    default:
        return true; // KNIGHT, QUEEN can threaten anything
    }
}

// Get attack bitboard for a piece type on a square (on an empty board for table construction)
// NOTE: side here uses threat-table convention (0=STM attacks upward, 1=NSTM attacks downward),
// which is the opposite of the engine's Side enum (BLACK=0 attacks downward, WHITE=1 attacks upward).
// We flip the side for pawn lookups so the table matches the Rust training code.
constexpr uint64_t piece_attacks_empty(PieceType pt, Side side, int sq)
{
    switch (pt)
    {
    case PAWN:
        return PawnAttacks[!side][sq];
    case KNIGHT:
        return KnightAttacks[sq];
    case BISHOP:
        return BishopAttacks[sq];
    case ROOK:
        return RookAttacks[sq];
    case QUEEN:
        return QueenAttacks[sq];
    case KING:
        return KingAttacks[sq];
    default:
        return 0;
    }
}

// ============================================================
// Compact Threat Table
// ============================================================

struct ThreatTable
{
    // Base offset for each attacker piece+side on each square
    uint32_t attacker_base[12][64];

    // Offset for victim piece+side within attacker-square bucket
    uint32_t victim_base[12][64][12];

    // Precomputed empty-board attack masks
    uint64_t attack_mask[12][64];

    uint32_t total_threat_features;
};

// ============================================================
// Build compact table (constexpr)
// ============================================================

constexpr ThreatTable build_threat_table()
{
    ThreatTable table {};

    uint32_t current_offset = 0;

    for (int atk_pt = 0; atk_pt < 6; atk_pt++)
    {
        auto atk_piece = static_cast<PieceType>(atk_pt);

        for (int atk_side = 0; atk_side < 2; atk_side++)
        {
            int atk_idx = atk_pt * 2 + atk_side;
            auto side = static_cast<Side>(atk_side);

            for (int atk_sq = 0; atk_sq < 64; atk_sq++)
            {
                // Pawns cannot exist on rank 0 or 7
                if (atk_piece == PAWN && (atk_sq / 8 == 0 || atk_sq / 8 == 7))
                {
                    table.attacker_base[atk_idx][atk_sq] = INVALID_THREAT;
                    table.attack_mask[atk_idx][atk_sq] = 0;

                    for (int v = 0; v < 12; v++)
                        table.victim_base[atk_idx][atk_sq][v] = INVALID_THREAT;

                    continue;
                }

                uint64_t mask = piece_attacks_empty(atk_piece, side, atk_sq);

                table.attack_mask[atk_idx][atk_sq] = mask;

                if (mask == 0)
                {
                    table.attacker_base[atk_idx][atk_sq] = INVALID_THREAT;

                    for (int v = 0; v < 12; v++)
                        table.victim_base[atk_idx][atk_sq][v] = INVALID_THREAT;

                    continue;
                }

                table.attacker_base[atk_idx][atk_sq] = current_offset;

                uint32_t victim_running_offset = 0;
                uint32_t attacks_per_victim = std::popcount(mask);

                for (int vic_pt = 0; vic_pt < 6; vic_pt++)
                {
                    auto vic_piece = static_cast<PieceType>(vic_pt);

                    for (int vic_side = 0; vic_side < 2; vic_side++)
                    {
                        int vic_idx = vic_pt * 2 + vic_side;

                        if (!can_threaten(atk_piece, vic_piece))
                        {
                            table.victim_base[atk_idx][atk_sq][vic_idx] = INVALID_THREAT;
                            continue;
                        }

                        table.victim_base[atk_idx][atk_sq][vic_idx] = victim_running_offset;

                        victim_running_offset += attacks_per_victim;
                    }
                }

                current_offset += victim_running_offset;
            }
        }
    }

    table.total_threat_features = current_offset;
    return table;
}

// ============================================================
// Global constexpr instance
// ============================================================

inline constexpr ThreatTable THREAT_TABLE = build_threat_table();
inline constexpr size_t TOTAL_THREAT_FEATURES = THREAT_TABLE.total_threat_features;

// ============================================================
// Runtime lookup
// ============================================================

constexpr uint32_t get_threat_index(int atk_idx, int atk_sq, int vic_idx, int vic_sq)
{
    uint32_t attacker_base = THREAT_TABLE.attacker_base[atk_idx][atk_sq];
    uint32_t victim_offset = THREAT_TABLE.victim_base[atk_idx][atk_sq][vic_idx];
    uint64_t mask = THREAT_TABLE.attack_mask[atk_idx][atk_sq];
    uint64_t target_bit = 1ULL << vic_sq;
    uint32_t square_rank = std::popcount(mask & (target_bit - 1));
    return attacker_base + victim_offset + square_rank;
}

template <Side perspective>
constexpr uint32_t get_threat_index(Piece atk_piece, Square atk_sq, Piece vic_piece, Square vic_sq, Square king_sq)
{
    constexpr int v_flip = perspective == WHITE ? 0 : 56;
    const int h_flip = enum_to<File>(king_sq) <= FILE_D ? 0 : 7;

    const auto atk_color = enum_to<Side>(atk_piece);
    const auto vic_color = enum_to<Side>(vic_piece);
    const auto atk_pt = enum_to<PieceType>(atk_piece);
    const auto vic_pt = enum_to<PieceType>(vic_piece);

    const int atk_side = (atk_color == perspective) ? 0 : 1;
    const int vic_side = (vic_color == perspective) ? 0 : 1;
    const int atk_idx = atk_pt * 2 + atk_side;
    const int vic_idx = vic_pt * 2 + vic_side;
    const uint32_t feat = get_threat_index(atk_idx, atk_sq ^ h_flip ^ v_flip, vic_idx, vic_sq ^ h_flip ^ v_flip);

    assert(feat != INVALID_THREAT);
    return feat;
}

} // namespace NN::Threats