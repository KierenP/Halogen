#pragma once

#include "bitboard/enum.h"
#include "movegen/move.h"
#include "search/score.h"
#include "spsa/tuneable.h"
#include "utility/fraction.h"

#include <algorithm>
#include <array>
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
};

struct PawnHistory : HistoryTable<PawnHistory>
{
    static TUNEABLE_CONSTANT int max_value = 7394;
    static TUNEABLE_CONSTANT int scale = 38;
    static constexpr size_t pawn_states = 512;
    int16_t table[N_SIDES][pawn_states][N_PIECE_TYPES][N_SQUARES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct ThreatHistory : HistoryTable<ThreatHistory>
{
    static TUNEABLE_CONSTANT int max_value = 11400;
    static TUNEABLE_CONSTANT int scale = 46;
    int16_t table[N_SIDES][2][2][N_SQUARES][N_SQUARES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct CaptureHistory : HistoryTable<CaptureHistory>
{
    static TUNEABLE_CONSTANT int max_value = 19241;
    static TUNEABLE_CONSTANT int scale = 51;
    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES][N_PIECE_TYPES] = {};
    int16_t* get(const BoardState& board, const SearchStackState* ss, Move move);
};

struct PieceMoveHistory : HistoryTable<PieceMoveHistory>
{
    static TUNEABLE_CONSTANT int max_value = 11814;
    static TUNEABLE_CONSTANT int scale = 37;
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
};

struct PawnCorrHistory
{
    // must be a power of 2, for fast hash lookup
    static constexpr size_t pawn_hash_table_size = 16384;
    static TUNEABLE_CONSTANT int correction_max = 95;
    static TUNEABLE_CONSTANT int scale = 85;

    int16_t table[N_SIDES][pawn_hash_table_size] = {};

    int16_t* get(const BoardState& board);
    const int16_t* get(const BoardState& board) const;

    void add(const BoardState& board, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board) const;

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
    static TUNEABLE_CONSTANT int correction_max = 99;
    static TUNEABLE_CONSTANT int scale = 174;

    int16_t table[N_SIDES][hash_table_size] = {};

    int16_t* get(const BoardState& board, Side side);
    void add(const BoardState& board, Side side, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board, Side side);

private:
    static int eval_scale()
    {
        return 16384 / correction_max;
    };
};

struct PieceMoveCorrHistory
{
    static TUNEABLE_CONSTANT int correction_max = 91;
    static TUNEABLE_CONSTANT int scale = 130;

    int16_t table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};

    int16_t* get(const BoardState& board, const SearchStackState* ss);
    void add(const BoardState& board, const SearchStackState* ss, int depth, int eval_diff);
    Score get_correction_score(const BoardState& board, const SearchStackState* ss);

private:
    static int eval_scale()
    {
        return 16384 / correction_max;
    };
};

struct ContinuationCorrHistory
{
    PieceMoveCorrHistory table[N_SIDES][N_PIECE_TYPES][N_SQUARES] = {};
};

struct SharedHistory
{
    PawnCorrHistory pawn_corr_hist;
    std::array<NonPawnCorrHistory, 2> non_pawn_corr;
    ContinuationCorrHistory cont_corr_hist;
};