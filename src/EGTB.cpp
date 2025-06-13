#include "EGTB.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <mutex>
#include <optional>

#include "BitBoardDefine.h"
#include "BoardState.h"
#include "Move.h"
#include "Pyrrhic/tbprobe.h"

Move extract_pyrrhic_move(const BoardState& board, PyrrhicMove move)
{
    const auto from = static_cast<Square>(PYRRHIC_MOVE_FROM(move));
    const auto to = static_cast<Square>(PYRRHIC_MOVE_TO(move));
    const auto flag = [&]()
    {
        auto flag_without_promo = board.GetMoveFlag(from, to);

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

std::optional<Score> Syzygy::probe_wdl_search(const BoardState& board, int distance_from_root)
{
    // Can't probe Syzygy if there is too many pieces on the board, if there is casteling rights, or fifty move isn't
    // zero
    if (board.fifty_move_count != 0 || GetBitCount(board.GetAllPieces()) > TB_LARGEST || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    // clang-format off
    auto probe = tb_probe_wdl(
        board.GetPieces<WHITE>(), 
        board.GetPieces<BLACK>(),
        board.GetPieceBB<KING>(),
        board.GetPieceBB<QUEEN>(),
        board.GetPieceBB<ROOK>(),
        board.GetPieceBB<BISHOP>(),
        board.GetPieceBB<KNIGHT>(),
        board.GetPieceBB<PAWN>(),
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
        return Score::draw();
    case TB_WIN:
        return Score::tb_win_in(distance_from_root);
    }

    assert(false);
    __builtin_unreachable();
}

std::optional<RootProbeResult> Syzygy::probe_dtz_root(const BoardState& board)
{
    // Can't probe Syzygy if there is too many pieces on the board, or if there is casteling rights.
    if (GetBitCount(board.GetAllPieces()) > TB_LARGEST || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    TbRootMoves root_moves {};
    static std::mutex tb_lock;

    /*
    TODO: Use the board cycle detection to correctly set the hasRepeated flag.
    */

    tb_lock.lock();
    // clang-format off
    auto ec = tb_probe_root_dtz(
        board.GetPieces<WHITE>(), 
        board.GetPieces<BLACK>(),
        board.GetPieceBB<KING>(),
        board.GetPieceBB<QUEEN>(),
        board.GetPieceBB<ROOK>(),
        board.GetPieceBB<BISHOP>(),
        board.GetPieceBB<KNIGHT>(),
        board.GetPieceBB<PAWN>(),
        board.fifty_move_count,
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE,
        false,
        &root_moves);
    // clang-format on
    tb_lock.unlock();

    // 0 means not all probes were successful
    if (!ec)
    {
        return std::nullopt;
    }

    std::ranges::stable_sort(root_moves.moves, std::greater<> {}, &TbRootMove::tbRank);
    RootProbeResult result;

    for (unsigned int i = 0; i < root_moves.size; i++)
    {
        result.root_moves.emplace_back(
            extract_pyrrhic_move(board, root_moves.moves[i].move), root_moves.moves[i].tbRank);
    }

    return result;
}
