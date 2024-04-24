#include "EGTB.h"
#include "BitBoardDefine.h"
#include "Move.h"
#include "Pyrrhic/tbprobe.h"

#include <iostream>

TbResult::TbResult(unsigned int result)
    : result_(result)
{
}

Score TbResult::get_score(int distance_from_root) const
{
    const auto tb_score = TB_GET_WDL(result_);

    switch (tb_score)
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

Move TbResult::get_move(const BoardState& board) const
{
    const auto from = static_cast<Square>(TB_GET_FROM(result_));
    const auto to = static_cast<Square>(TB_GET_TO(result_));
    const auto flag = [&]()
    {
        auto flag_without_promo = board.GetMoveFlag(from, to);
        const auto promo = TB_GET_PROMOTES(result_);

        if (promo == PYRRHIC_PROMOTES_KNIGHT)
        {
            return (flag_without_promo == CAPTURE ? KNIGHT_PROMOTION_CAPTURE : KNIGHT_PROMOTION);
        }
        if (promo == PYRRHIC_PROMOTES_BISHOP)
        {
            return (flag_without_promo == CAPTURE ? BISHOP_PROMOTION_CAPTURE : BISHOP_PROMOTION);
        }
        if (promo == PYRRHIC_PROMOTES_ROOK)
        {
            return (flag_without_promo == CAPTURE ? ROOK_PROMOTION_CAPTURE : ROOK_PROMOTION);
        }
        if (promo == PYRRHIC_PROMOTES_QUEEN)
        {
            return (flag_without_promo == CAPTURE ? QUEEN_PROMOTION_CAPTURE : QUEEN_PROMOTION);
        }

        return flag_without_promo;
    }();

    return Move(from, to, flag);
}

void Syzygy::init(std::string_view path)
{
    tb_init(path.data());
    std::cout << "info string Found " << TB_NUM_WDL << " WDL, " << TB_NUM_DTM << " DTM and " << TB_NUM_DTZ
              << " DTZ tablebase files. Largest " << TB_LARGEST << "-men" << std::endl;
}

std::optional<TbResult> Syzygy::probe_wdl_search(const BoardState& board)
{
    // Can't probe Syzygy if there is too many pieces on the board, if there is casteling rights, or fifty move isn't
    // zero
    if (board.fifty_move_count != 0 || GetBitCount(board.GetAllPieces()) > TB_LARGEST || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    // clang-format off
    auto probe = tb_probe_wdl(
        board.GetWhitePieces(), 
        board.GetBlackPieces(),
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

    return probe;
}

std::optional<RootProbeResult> Syzygy::probe_dtz_root(const BoardState& board)
{
    // Can't probe Syzygy if there is too many pieces on the board, or if there is casteling rights.
    if (GetBitCount(board.GetAllPieces()) > TB_LARGEST || board.castle_squares != EMPTY)
    {
        return std::nullopt;
    }

    uint32_t move_results[256];

    // clang-format off
    auto probe = tb_probe_root(
        board.GetWhitePieces(), 
        board.GetBlackPieces(),
        board.GetPieceBB<KING>(),
        board.GetPieceBB<QUEEN>(),
        board.GetPieceBB<ROOK>(),
        board.GetPieceBB<BISHOP>(),
        board.GetPieceBB<KNIGHT>(),
        board.GetPieceBB<PAWN>(),
        board.fifty_move_count,
        board.en_passant <= SQ_H8 ? board.en_passant : 0,
        board.stm == WHITE,
        move_results);
    // clang-format on

    if (probe == TB_RESULT_FAILED || probe == TB_RESULT_STALEMATE || probe == TB_RESULT_CHECKMATE)
    {
        return std::nullopt;
    }

    RootProbeResult result;
    result.root_result_ = probe;

    // filter out the results which preserve the WDL outcome
    for (int i = 0; i < 256 && move_results[i] != TB_RESULT_FAILED; i++)
    {
        if (TB_GET_WDL(probe) == TB_GET_WDL(move_results[i]))
        {
            result.root_move_whitelist.emplace_back(TbResult(move_results[i]).get_move(board));
        }
    }

    return result;
}
