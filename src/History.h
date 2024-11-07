#pragma once

#include "BitBoardDefine.h"
#include "GameState.h"

#include <cstdint>
#include <tuple>

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
    static constexpr int max_value = 12776;
    static constexpr int scale = 42;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_PLAYERS][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct ThreatHistory : HistoryTable<ThreatHistory>
{
    static constexpr int max_value = 7158;
    static constexpr int scale = 55;
    int16_t table[N_PLAYERS][2][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static constexpr int max_value = 12911;
    static constexpr int scale = 30;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct PieceMoveHistory : HistoryTable<PieceMoveHistory>
{
    static constexpr int max_value = 6512;
    static constexpr int scale = 100;
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
    static std::array<PieceMoveHistory*, cont_hist_depth> get_subtables(const SearchStackState* ss);
    void add(const GameState& position, const SearchStackState* ss, Move move, int change);
    int32_t get(const GameState& position, const SearchStackState* ss, Move move);

    constexpr void reset()
    {
        memset(table, 0, sizeof(table));
    }
};

template <typename... tables>
class History
{
public:
    void reset()
    {
        std::apply([](auto&... table) { (table.reset(), ...); }, tables_);
    }

    int get(const GameState& position, const SearchStackState* ss, Move move)
    {
        auto get_value = [&](auto& table)
        {
            auto* value = table.get(position, ss, move);
            return value ? *value : 0;
        };

        return std::apply([&](auto&... table) { return (get_value(table) + ...); }, tables_);
    }

    void add(const GameState& position, const SearchStackState* ss, Move move, int change)
    {
        std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
    }

private:
    std::tuple<tables...> tables_;
};

using QuietHistory = History<PawnHistory, ThreatHistory>;
using LoudHistory = History<CaptureHistory>;