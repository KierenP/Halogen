#pragma once

#include "bitboard/enum.h"
#include "movegen/move.h"
#include "search/score.h"
#include "spsa/tuneable.h"
#include "utility/fraction.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

class BoardState;
struct SearchStackState;

template <typename Derived>
struct HistoryTable
{
    static constexpr void adjust_history(int16_t& entry, Fraction<64> change)
    {
        int adjust = std::clamp((change * Derived::scale).to_int(), -Derived::max_value / 4, Derived::max_value / 4);
        entry += (adjust - entry * abs(adjust) / Derived::max_value);
    }

    constexpr void add(const BoardState& board, const SearchStackState* ss, Move move, Fraction<64> change)
    {
        auto* entry = static_cast<Derived&>(*this).get(board, ss, move);
        if (!entry)
        {
            return;
        }
        adjust_history(*entry, change);
    }

    constexpr void reset()
    {
        auto& table = static_cast<Derived&>(*this).table;
        memset(table, 0, sizeof(table));
    }
};

struct PawnHistory : HistoryTable<PawnHistory>
{
    static TUNEABLE_CONSTANT int max_value = 9211;
    static TUNEABLE_CONSTANT int scale = 37;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_SIDES][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct ThreatHistory : HistoryTable<ThreatHistory>
{
    static TUNEABLE_CONSTANT int max_value = 11947;
    static TUNEABLE_CONSTANT int scale = 46;
    int16_t table[N_SIDES][2][2][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static TUNEABLE_CONSTANT int max_value = 17837;
    static TUNEABLE_CONSTANT int scale = 49;
    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct PieceMoveHistory : HistoryTable<PieceMoveHistory>
{
    static TUNEABLE_CONSTANT int max_value = 12277;
    static TUNEABLE_CONSTANT int scale = 36;
    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

// Continuation history lets us look up history in the form
// [side][(ss-N)->moved_piece][(ss-N)->move.to()][(ss)->moved_piece][(ss)->move.to()]
// To add strength, we use the same table for all values of N (typically 1..6 in strong engines).
// This means if we add a bonus for a particular 4 ply continuation, we will also observe this bonus for
// equivalent 2 ply or 6 ply continuations
struct ContinuationHistory
{
    static constexpr int cont_hist_depth = 2;
    PieceMoveHistory table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }
};

struct PawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t pawn_hash_table_size = 16384;
    static TUNEABLE_CONSTANT int correction_max = 93;
    static TUNEABLE_CONSTANT int scale = 82;

    int16_t table[N_SIDES][pawn_hash_table_size] = {};

    int16_t* get(const BoardState& board);
    const int16_t* get(const BoardState& board) const;

    void add(const BoardState& board, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board) const;

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static int eval_scale()
    {
        return 16384 / correction_max;
    };
};

struct NonPawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t hash_table_size = 16384;
    static TUNEABLE_CONSTANT int correction_max = 94;
    static TUNEABLE_CONSTANT int scale = 166;

    int16_t table[N_SIDES][hash_table_size] = {};

    int16_t* get(const BoardState& board, Side side);
    void add(const BoardState& board, Side side, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board, Side side);

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static int eval_scale()
    {
        return 16384 / correction_max;
    };
};

struct PieceMoveCorrHistory
{
    static TUNEABLE_CONSTANT int correction_max = 88;
    static TUNEABLE_CONSTANT int scale = 123;

    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};

    int16_t* get(const BoardState& board, const SearchStackState* ss);
    void add(const BoardState& board, const SearchStackState* ss, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board, const SearchStackState* ss);

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static int eval_scale()
    {
        return 16384 / correction_max;
    };
};

struct ContinuationCorrHistory
{
    PieceMoveCorrHistory table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }
};

// Idea from Winter
//
// Random Slice History (RSH):
// Each piece-square is assigned a 64-bit "Zobrist" value with exactly one non-zero 16-bit slice.
// The slice is placed randomly in one of the 4 possible 16-bit blocks: [0-15], [16-31], [32-47], or [48-63].
// XORing all pieces on the board produces a 64-bit board key, which is then split into 4 slices.
// Each slice indexes its own correction history table, effectively creating 4 overlapping
// “views” of the board. This preserves locality: similar positions share most slices,
// allowing corrections to generalize. This scheme is analogous to pawn/non-pawn/minor/major tables,
// but the partitions are chosen randomly rather than by hand.
//
struct RandomSliceHistory
{
    static constexpr size_t slice_table_size = 65536;
    static TUNEABLE_CONSTANT int correction_max = 93;
    static TUNEABLE_CONSTANT int scale = 82;

    // 2 sides × 4 slices × 65536 entries
    int16_t table[N_SIDES][4][slice_table_size] = {};

    void add(const BoardState& board, int depth, int eval_diff);

    Score get_correction_score(const BoardState& board) const;

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static int eval_scale()
    {
        return slice_table_size / correction_max;
    }
};