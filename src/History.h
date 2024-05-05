#pragma once

#include "BitBoardDefine.h"
#include "GameState.h"

#include <array>
#include <cstdint>
#include <tuple>

struct SearchStackState;

template <typename Derived>
struct HistoryTable
{
    static constexpr void adjust_history(int16_t& entry, int change)
    {
        entry += Derived::scale * change - entry * abs(change) * Derived::scale / Derived::max_value;
    }

    constexpr void add(const GameState& position, const SearchStackState* ss, Move move, int change)
    {
        if (static_cast<Derived&>(*this).valid(position, ss, move))
        {
            auto hist = static_cast<Derived&>(*this).get(position, ss, move);
            adjust_history(hist, change);
        }
    }

    constexpr void reset()
    {
        auto& table = static_cast<Derived&>(*this).table;
        memset(table, 0, sizeof(table));
    }
};

struct ButterflyHistory : HistoryTable<ButterflyHistory>
{
    static constexpr int max_value = 16384;
    static constexpr int scale = 32;
    int16_t table[N_PLAYERS][N_SQUARES][N_SQUARES] = {};
    int16_t& get(const GameState& position, const SearchStackState* ss, Move move);
    bool valid(const GameState& position, const SearchStackState* ss, Move move);
};

struct CountermoveHistory : HistoryTable<CountermoveHistory>
{
    static constexpr int max_value = 16384;
    static constexpr int scale = 64;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t& get(const GameState& position, const SearchStackState* ss, Move move);
    bool valid(const GameState& position, const SearchStackState* ss, Move move);
};

class History
{
public:
    void reset();
    int get(const GameState& position, const SearchStackState* ss, Move move);
    void add(const GameState& position, const SearchStackState* ss, Move move, int change);

private:
    std::tuple<ButterflyHistory, CountermoveHistory> tables_;
};