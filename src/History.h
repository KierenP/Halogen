#pragma once

#include "BitBoardDefine.h"
#include "GameState.h"
#include "SearchStack.h"

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
    static constexpr int max_value = 16384;
    static constexpr int scale = 32;
    int16_t table[N_PLAYERS][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
};

template <size_t depth>
struct CountermoveHistory : HistoryTable<CountermoveHistory<depth>>
{
    static constexpr int max_value = 16384;
    static constexpr int scale = 64;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move)
    {
        const auto& counter_move = (ss - depth)->move;
        const auto& counter_piece = (ss - depth)->moved_piece;

        if (counter_move == Move::Uninitialized || counter_piece == N_PIECES)
        {
            return nullptr;
        }

        const auto& stm = position.Board().stm;
        const auto curr_piece = GetPieceType(position.Board().GetSquare(move.GetFrom()));

        return &table[stm][counter_piece][counter_move.GetTo()][curr_piece][move.GetTo()];
    }
};

template <typename... Tables>
class History
{
public:
    void reset()
    {
        std::apply([](auto&... table) { (table.reset(), ...); }, tables_);
    }

    int16_t get(const GameState& position, const SearchStackState* ss, Move move)
    {
        auto get_value = [&](auto& table) -> int
        {
            auto* value = table.get(position, ss, move);
            return value ? *value : 0;
        };

        return std::apply([&](auto&... table) { return (get_value(table) + ...); }, tables_)
            / (int)std::tuple_size_v<decltype(tables_)>;
    }

    void add(const GameState& position, const SearchStackState* ss, Move move, int change)
    {
        std::apply([&](auto&... table) { (table.add(position, ss, move, change), ...); }, tables_);
    }

private:
    std::tuple<Tables...> tables_;
};

using QuietHistory = History<ButterflyHistory>;
using LoudHistory = History<ButterflyHistory, CountermoveHistory<1>>;
