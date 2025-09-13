#include "search/syzygy.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <mutex>
#include <optional>

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "chessboard/game_state.h"
#include "movegen/move.h"
#include "third-party/Pyrrhic/tbprobe.h"

Move extract_pyrrhic_move(const BoardState& board, PyrrhicMove move)
{
    const auto from = static_cast<Square>(PYRRHIC_MOVE_FROM(move));
    const auto to = static_cast<Square>(PYRRHIC_MOVE_TO(move));
    const auto flag = [&]()
    {
        auto flag_without_promo = board.infer_move_flag(from, to);

        if (PYRRHIC_MOVE_IS_NPROMO(move))
        {
            return (flag_without_promo == CAPTURE ? KNIGHT_PROMOTION_CAPTURE : KNIGHT_PROMOTION);
        }
        if (PYRRHIC_MOVE_IS_BPROMO(move))
        {
            return (flag_without_promo == CAPTURE ? BISHOP_PROMOTION_CAPTURE : BISHOP_PROMOTION);
        }
        if (PYRRHIC_MOVE_IS_RPROMO(move))
        {
            return (flag_without_promo == CAPTURE ? ROOK_PROMOTION_CAPTURE : ROOK_PROMOTION);
        }
        if (PYRRHIC_MOVE_IS_QPROMO(move))
        {
            return (flag_without_promo == CAPTURE ? QUEEN_PROMOTION_CAPTURE : QUEEN_PROMOTION);
        }

        return flag_without_promo;
    }();

    return { from, to, flag };
}

void Syzygy::init(std::string_view path, bool print)
{
    tb_init(path.data());

    if (print)
    {
        std::cout << "info string Found " << TB_NUM_WDL << " WDL, " << TB_NUM_DTM << " DTM and " << TB_NUM_DTZ
                  << " DTZ tablebase files. Largest " << TB_LARGEST << "-men" << std::endl;
    }
}

std::optional<Score> Syzygy::probe_wdl_search(const SearchLocalState& local, int distance_from_root)
{
    // Can't probe Syzygy if there is too many pieces on the board, if there is casteling rights, or fifty move isn't
    // zero
    const auto& board = local.position.board();
    if (board.fifty_move_count != 0 || std::popcount(board.get_pieces_bb()) > TB_LARGEST
        || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    // clang-format off
    auto probe = tb_probe_wdl(
        board.get_pieces_bb(WHITE), 
        board.get_pieces_bb(BLACK),
        board.get_pieces_bb(KING),
        board.get_pieces_bb(QUEEN),
        board.get_pieces_bb(ROOK),
        board.get_pieces_bb(BISHOP),
        board.get_pieces_bb(KNIGHT),
        board.get_pieces_bb(PAWN),
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE);
    // clang-format on

    if (probe == TB_RESULT_FAILED)
    {
        return std::nullopt;
    }

    switch (probe)
    {
    case TB_LOSS:
        return Score::tb_loss_in(distance_from_root);
    case TB_BLESSED_LOSS:
    case TB_DRAW:
    case TB_CURSED_WIN:
        return Score::draw_random(local.nodes);
    case TB_WIN:
        return Score::tb_win_in(distance_from_root);
    }

    assert(false);
    __builtin_unreachable();
}

std::optional<RootProbeResult> Syzygy::probe_dtz_root(const GameState& position)
{
    const auto& board = position.board();

    // Can't probe Syzygy if there is too many pieces on the board, or if there is casteling rights.
    if (std::popcount(board.get_pieces_bb()) > TB_LARGEST || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    TbRootMoves root_moves {};
    static std::mutex tb_lock;

    tb_lock.lock();
    // clang-format off
    auto ec = tb_probe_root_dtz(
        board.get_pieces_bb(WHITE), 
        board.get_pieces_bb(BLACK),
        board.get_pieces_bb(KING),
        board.get_pieces_bb(QUEEN),
        board.get_pieces_bb(ROOK),
        board.get_pieces_bb(BISHOP),
        board.get_pieces_bb(KNIGHT),
        board.get_pieces_bb(PAWN),
        board.fifty_move_count,
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE,
        position.has_repeated(),
        &root_moves);
    // clang-format on
    tb_lock.unlock();

    // 0 means not all probes were successful
    if (!ec)
    {
        return std::nullopt;
    }

    auto probe_moves = std::ranges::subrange { root_moves.moves, root_moves.moves + root_moves.size };
    std::ranges::stable_sort(probe_moves, std::greater<> {}, &TbRootMove::tbRank);
    RootProbeResult result;

    for (const auto& move : probe_moves)
    {
        result.root_moves.emplace_back(extract_pyrrhic_move(board, move.move), move.tbRank);
    }

    return result;
}
