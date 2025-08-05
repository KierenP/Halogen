#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <utility>
#include <vector>

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/game_state.h"
#include "movegen/list.h"
#include "movegen/move.h"
#include "network/network.h"
#include "search/history.h"
#include "search/limit/limits.h"
#include "search/limit/time.h"
#include "search/score.h"
#include "search/transposition/table.h"
#include "utility/atomic.h"
#include "utility/static_vector.h"

namespace Transposition
{
class Table;
}

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

namespace UCI
{
class UciOutput;
}

// Holds information about the search state for a particular recursion depth.
struct SearchStackState
{
    SearchStackState(int distance_from_root_);

    StaticVector<Move, MAX_DEPTH> pv = {};

    Move move = Move::Uninitialized;
    Piece moved_piece = N_PIECES;
    std::array<uint64_t, N_PIECE_TYPES> threat_mask = {};
    uint64_t threat_hash = 0;

    Move singular_exclusion = Move::Uninitialized;

    int nmp_verification_depth = 0;
    bool nmp_verification_root = false;

    int distance_from_root;

    PieceMoveHistory* cont_hist_subtable = nullptr;
    PieceMoveCorrHistory* cont_corr_hist_subtable = nullptr;

    NN::Accumulator acc;

    Score adjusted_eval = 0;
};

class SearchStack
{
    // The search accesses [ss-4, ss+1]
    constexpr static int min_access = -4;
    constexpr static int max_access = 1;
    constexpr static size_t size = MAX_DEPTH + max_access - min_access;

public:
    const SearchStackState* root() const;
    SearchStackState* root();

    SearchStack() = default;

private:
    std::array<SearchStackState, size> search_stack_array_ { generate(std::make_integer_sequence<int, size>()) };

    template <int... distances_from_root>
    decltype(search_stack_array_) generate(std::integer_sequence<int, distances_from_root...>)
    {
        return { (distances_from_root + min_access)... };
    }
};

struct RootMove
{
    RootMove() = default;
    RootMove(Move move_)
        : move(move_)
    {
    }

    Move move = Move::Uninitialized;
    int64_t nodes = 0;
    Score score = SCORE_UNDEFINED;
};

// Data local to a particular thread
struct alignas(hardware_destructive_interference_size) SearchLocalState
{
public:
    SearchLocalState(int thread_id);

    bool should_skip_root_move(Move move);
    void reset_new_search();

    int get_quiet_search_history(const SearchStackState* ss, Move move);
    int get_quiet_order_history(const SearchStackState* ss, Move move);
    int get_loud_history(const SearchStackState* ss, Move move);

    void add_quiet_history(const SearchStackState* ss, Move move, int change);
    void add_loud_history(const SearchStackState* ss, Move move, int change);

    int thread_id;
    SearchStack search_stack;
    PawnHistory pawn_hist;
    ThreatHistory threat_hist;
    ContinuationHistory cont_hist;
    CaptureHistory capt_hist;
    PawnCorrHistory pawn_corr_hist;
    std::array<NonPawnCorrHistory, 2> non_pawn_corr;
    ContinuationCorrHistory cont_corr_hist;
    ThreatCorrHistory threat_corr_hist;
    int sel_septh = 0;
    AtomicRelaxed<int64_t> tb_hits = 0;
    AtomicRelaxed<int64_t> nodes = 0;

    // track the current depth + multi-pv of the search
    int curr_depth = 0;
    int curr_multi_pv = 0;

    // Set to true when the search is unwinding and trying to return.
    bool aborting_search = false;

    // If we don't think we can complete the next depth within the iterative deepening loop before running out of time,
    // we want to stop the search early and save the leftover time. When multiple threads are involved, we don't want
    // the threads stopping early if other threads are continuing. This signal lets the SearchSharedData check which
    // threads want to stop and if all threads do then we stop the search. TODO: could use a consensus model?
    AtomicRelAcq<bool> thread_wants_to_stop = false;

    // If set, these restrict the possible root moves considered. A root move will be skipped if it is present in the
    // blacklist, or if it is missing from the whitelist (unless whitelist is empty)
    BasicMoveList root_move_whitelist {};
    BasicMoveList root_move_blacklist {};

    StaticVector<RootMove, 256> root_moves;

    // Each time we check the time remaining, we reset this counter to schedule a later time to recheck
    int limit_check_counter = 0;

    NN::Network net;

    GameState position = GameState::starting_position();
};

struct SearchResults
{
    int depth = 0;
    int sel_septh = 0;
    int multi_pv = 0;
    int64_t nodes = 0;
    Score score = SCORE_UNDEFINED;
    StaticVector<Move, MAX_DEPTH> pv = {};
    SearchResultType type = SearchResultType::EMPTY;
};

// Search state that is shared between threads.
class SearchSharedState
{
public:
    SearchSharedState(UCI::UciOutput& uci);

    // Below functions are not thread-safe and should not be called during search
    // ------------------------------------

    void reset_new_search();
    void reset_new_game();
    void set_multi_pv(int multi_pv);
    void set_threads(int threads);
    void set_hash(int hash_size_mb);

    // Below functions are thread-safe and blocking
    // ------------------------------------

    SearchResults get_best_search_result() const;

    void report_search_result(
        const SearchStackState* ss, const SearchLocalState& local, Score score, SearchResultType type);

    // Below functions are thread-safe and non-blocking
    // ------------------------------------

    int64_t tb_hits() const;
    int64_t nodes() const;
    int get_threads_setting() const;
    int get_multi_pv_setting() const;

    void report_thread_wants_to_stop();

    bool chess_960 {};
    SearchLimits limits;
    Timer search_timer;
    UCI::UciOutput& uci_handler;

    AtomicRelaxed<bool> stop_searching = false;
    Transposition::Table transposition_table;

    std::vector<const SearchLocalState*> search_local_states_;

private:
    mutable std::recursive_mutex lock_;
    int multi_pv_setting {};
    int threads_setting {};

    // [thread_id][multi_pv][depth]
    std::vector<std::vector<std::array<SearchResults, MAX_DEPTH>>> search_results_;

    std::optional<SearchResults> best_search_result_;
};
