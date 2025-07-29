#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include "bitboard/define.h"
#include "movegen/move.h"
#include "search/score.h"
#include "spsa/tuneable.h"

class GameState;
struct SearchStackState;

template <typename Derived>
struct HistoryTable
{
    static constexpr void adjust_history(int16_t& entry, int change)
    {
        change = std::clamp(change, -Derived::max_value / Derived::scale, Derived::max_value / Derived::scale);
        entry += Derived::scale * change - entry * abs(change) * Derived::scale / Derived::max_value;
    }

    constexpr void add(const GameState& position, const SearchStackState* ss, Move move, int change)
    {
        auto* entry = static_cast<Derived&>(*this).get(position, ss, move);
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
    static TUNEABLE_CONSTANT int max_value = 8335;
    static TUNEABLE_CONSTANT int scale = 37;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_SIDES][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct ThreatHistory : HistoryTable<ThreatHistory>
{
    static TUNEABLE_CONSTANT int max_value = 5113;
    static TUNEABLE_CONSTANT int scale = 41;
    int16_t table[N_SIDES][2][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static TUNEABLE_CONSTANT int max_value = 19616;
    static TUNEABLE_CONSTANT int scale = 40;
    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct PieceMoveHistory : HistoryTable<PieceMoveHistory>
{
    static TUNEABLE_CONSTANT int max_value = 9270;
    static TUNEABLE_CONSTANT int scale = 35;
    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
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

TUNEABLE_CONSTANT int corr_hist_scale = 134;

struct PawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t pawn_hash_table_size = 16384;
    static TUNEABLE_CONSTANT int correction_max = 59;

    int16_t table[N_SIDES][pawn_hash_table_size] = {};

    int16_t* get(const GameState& position);
    const int16_t* get(const GameState& position) const;

    void add(const GameState& position, int depth, int eval_diff);
    Score get_correction_score(const GameState& position) const;

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
    static TUNEABLE_CONSTANT int correction_max = 82;

    int16_t table[N_SIDES][hash_table_size] = {};

    int16_t* get(const GameState& position, Side side);
    void add(const GameState& position, Side side, int depth, int eval_diff);
    Score get_correction_score(const GameState& position, Side side);

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
