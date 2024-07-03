#pragma once

#include "BitBoardDefine.h"
#include "GameState.h"

#include <cstdint>
#include <limits>
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

struct CountermoveHistory : HistoryTable<CountermoveHistory>
{
    static constexpr int max_value = 16384;
    static constexpr int scale = 64;
    int16_t table[N_PLAYERS][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const GameState& position, const SearchStackState* ss, Move move);
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

// A special type of history not used for move ordering, but tracks a EWA of the difference between static eval and
// search score
struct CorrectionHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t pawn_hash_table_size = 16384;
    static constexpr Score correction_max = 32;
    static constexpr int eval_scale = 512;
    static constexpr int ewa_weight_scale = 256;
    static_assert(correction_max.value() * eval_scale < std::numeric_limits<int16_t>::max());

    int16_t table[N_PLAYERS][pawn_hash_table_size] = {};

    int16_t* get(const GameState& position);
    void add(const GameState& position, int depth, int eval_diff);
    void reset();

    Score get_correction_score(const GameState& position);
};
