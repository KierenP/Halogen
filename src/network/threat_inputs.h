#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace NN
{

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

constexpr uint32_t INVALID_THREAT = UINT32_MAX;

// Deduplication: for symmetric attack patterns, keep (atk_idx, atk_sq) < (vic_idx, vic_sq).
// Symmetric pairs: knight-knight, bishop-bishop, rook-rook, queen-queen, pawn-pawn (opposite sides).
// Cross-type slider threats: bishop/rook do NOT threaten queens; only queens threaten bishops/rooks.
// King only threatens pawns, knights, bishops, rooks.
constexpr bool is_duplicate_threat(
    PieceType atk_piece, Side atk_side, int atk_sq, PieceType vic_piece, Side vic_side, int vic_sq)
{
    bool symmetric = false;
    if (atk_piece == vic_piece)
    {
        switch (atk_piece)
        {
        case KNIGHT:
        case BISHOP:
        case ROOK:
        case QUEEN:
            symmetric = true;
            break;
        case PAWN:
            symmetric = (atk_side != vic_side);
            break;
        default:
            break;
        }
    }

    if (!symmetric)
        return false;

    int atk_idx = atk_side * 6 + atk_piece;
    int vic_idx = vic_side * 6 + vic_piece;

    return (atk_idx > vic_idx) || (atk_idx == vic_idx && atk_sq > vic_sq);
}

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
        return BishopAttacks[sq]; // full rays on empty board
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

// Precomputed threat lookup tables
struct ThreatTable
{
    // Direct lookup: [atk_idx][atk_sq][vic_idx][vic_sq] -> feature index
    // atk_idx = piece_type * 2 + side, vic_idx = piece_type * 2 + side
    uint32_t lookup[12][64][12][64];

    // Total number of threat features (per perspective)
    uint32_t total_threat_features;
};

constexpr ThreatTable build_threat_table()
{
    ThreatTable table = {};
    // Initialize all to invalid
    for (int a = 0; a < 12; a++)
        for (int b = 0; b < 64; b++)
            for (int c = 0; c < 12; c++)
                for (int d = 0; d < 64; d++)
                    table.lookup[a][b][c][d] = INVALID_THREAT;

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
                // Pawns can't be on rank 0 or rank 7
                if (atk_piece == PAWN && (atk_sq / 8 == 0 || atk_sq / 8 == 7))
                    continue;

                uint64_t attack_bb = piece_attacks_empty(atk_piece, side, atk_sq);
                if (attack_bb == 0)
                    continue;

                for (int vic_pt = 0; vic_pt < 6; vic_pt++)
                {
                    auto vic_piece = static_cast<PieceType>(vic_pt);
                    for (int vic_side = 0; vic_side < 2; vic_side++)
                    {
                        int vic_idx = vic_pt * 2 + vic_side;

                        if (!can_threaten(atk_piece, vic_piece))
                            continue;

                        uint64_t bb = attack_bb;
                        while (bb)
                        {
                            int target_sq = std::countr_zero(bb);
                            bb &= bb - 1;

                            if (is_duplicate_threat(
                                    atk_piece, side, atk_sq, vic_piece, static_cast<Side>(vic_side), target_sq))
                                continue;

                            table.lookup[atk_idx][atk_sq][vic_idx][target_sq] = current_offset;
                            current_offset++;
                        }
                    }
                }
            }
        }
    }

    table.total_threat_features = current_offset;
    return table;
}

// Compile-time threat table
inline constexpr ThreatTable THREAT_TABLE = build_threat_table();

// Total number of threat features
inline constexpr size_t TOTAL_THREAT_FEATURES = THREAT_TABLE.total_threat_features;

// ============================================================
// Input layout constants
// ============================================================

// Feature layout in the weight matrix:
//   [0, 768 * KING_BUCKET_COUNT)                           : king-bucketed piece-square
//   [768 * KING_BUCKET_COUNT, 768 * KING_BUCKET_COUNT+768) : unbucketed piece-square
//   [768 * KING_BUCKET_COUNT + 768, ...)                   : threat features
inline constexpr size_t KING_BUCKET_INPUT_COUNT = 768; // 12 pieces * 64 squares
inline constexpr size_t PSQT_INPUT_COUNT = 768; // 12 pieces * 64 squares (unbucketed)

}
