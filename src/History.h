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

struct ButterflyHistory : HistoryTable<ButterflyHistory>
{
    static constexpr int max_value = 12116;
    static constexpr int scale = 57;
    int16_t table[N_PLAYERS][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct CountermoveHistory : HistoryTable<CountermoveHistory>
{
    static constexpr int max_value = 12546;
    static constexpr int scale = 44;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct FollowmoveHistory : HistoryTable<FollowmoveHistory>
{
    static constexpr int max_value = 12546;
    static constexpr int scale = 44;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct PawnHistory : HistoryTable<PawnHistory>
{
    static constexpr int max_value = 12546;
    static constexpr int scale = 44;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_PLAYERS][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static constexpr int max_value = 18795;
    static constexpr int scale = 40;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
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

using QuietHistory = History<ButterflyHistory, CountermoveHistory, FollowmoveHistory, PawnHistory>;
using LoudHistory = History<CaptureHistory>;