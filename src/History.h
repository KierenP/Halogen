#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <tuple>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Score.h"

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
    static constexpr int max_value = 10036;
    static constexpr int scale = 35;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_PLAYERS][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct ThreatHistory : HistoryTable<ThreatHistory>
{
    static constexpr int max_value = 9692;
    static constexpr int scale = 45;
    int16_t table[N_PLAYERS][2][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static constexpr int max_value = 18795;
    static constexpr int scale = 40;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct PieceMoveHistory : HistoryTable<PieceMoveHistory>
{
    static constexpr int max_value = 10036;
    static constexpr int scale = 35;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

// Continuation history lets us look up history in the form
// [side][(ss-N)->moved_piece][(ss-N)->move.GetTo()][(ss)->moved_piece][(ss)->move.GetTo()]
// To add strength, we use the same table for all values of N (typically 1..6 in strong engines).
// This means if we add a bonus for a particular 4 ply continuation, we will also observe this bonus for
// equivalent 2 ply or 6 ply continuations
struct ContinuationHistory
{
    static constexpr int cont_hist_depth = 2;
    PieceMoveHistory table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES] = {};

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }
};

struct PawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t pawn_hash_table_size = 16384;
    static constexpr int correction_max = 16;

    int16_t table[N_PLAYERS][pawn_hash_table_size] = {};

    int16_t* get(const GameState& position);
    const int16_t* get(const GameState& position) const;

    void add(const GameState& position, int depth, int eval_diff);
    Score get_correction_score(const GameState& position) const;

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static constexpr int eval_scale = 16384 / correction_max;
};

struct NonPawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t hash_table_size = 16384;
    static constexpr int correction_max = 16;

    int16_t table[N_PLAYERS][hash_table_size] = {};

    int16_t* get(const GameState& position, Players side);
    void add(const GameState& position, Players side, int depth, int eval_diff);
    Score get_correction_score(const GameState& position, Players side);

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }

private:
    static constexpr int eval_scale = 16384 / correction_max;
};
