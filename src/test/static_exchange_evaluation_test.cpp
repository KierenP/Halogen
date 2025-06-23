#include <cassert>

#include "bitboard/define.h"
#include "chessboard/game_state.h"
#include "movegen/move.h"
#include "search/score.h"
#include "search/static_exchange_evaluation.h"

const auto test_see
    = []([[maybe_unused]] const GameState& position, [[maybe_unused]] Move move, [[maybe_unused]] Score expected_value)
{
    assert(see(position.board(), move) == expected_value.value());
    assert(see_ge(position.board(), move, expected_value));
    assert(!see_ge(position.board(), move, expected_value + 1));
};

void static_exchange_evaluation_test()
{
    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/4p3/1b2P3/2P4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), PieceValues[BISHOP]);
    }

    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/2p1p3/1b2P3/2P4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), PieceValues[BISHOP] - PieceValues[PAWN]);
    }

    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/2p1p3/1b2P3/2Q4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), PieceValues[BISHOP] - PieceValues[QUEEN]);
    }

    {
        auto position = GameState::from_fen("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -");
        test_see(position, Move(SQ_D3, SQ_E5, CAPTURE), PieceValues[PAWN] - PieceValues[KNIGHT]);
    }

    {
        auto position = GameState::from_fen("1k1r4/1pp4p/p2b1b2/4q3/8/P3R1P1/1PP1R1BP/2K1Q3 w - - 0 1");
        test_see(position, Move(SQ_E3, SQ_E5, CAPTURE),
            PieceValues[QUEEN] - PieceValues[ROOK] + PieceValues[BISHOP] - PieceValues[ROOK] + PieceValues[BISHOP]);
    }

    // en-passant tests, with discovered attacks

    {
        auto position = GameState::from_fen("1k6/8/4R3/6B1/2n1Pp2/8/8/1K6 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT),
            PieceValues[PAWN] - PieceValues[PAWN] + PieceValues[BISHOP] - PieceValues[KNIGHT]);
    }

    // king attacks

    {
        auto position = GameState::from_fen("1k6/4r3/8/8/8/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E7, SQ_E3, CAPTURE), PieceValues[PAWN] - PieceValues[ROOK]);
    }

    {
        auto position = GameState::from_fen("1k2r3/4r3/8/8/8/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E7, SQ_E3, CAPTURE), PieceValues[PAWN]);
    }

    {
        auto position = GameState::from_fen("4r3/8/8/8/3k4/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E8, SQ_E3, CAPTURE), PieceValues[PAWN]);
    }

    // king attacks + en passant + discovered attacks

    {
        auto position = GameState::from_fen("1k6/8/8/3n2B1/4Pp2/8/3K4/8 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT),
            PieceValues[PAWN] - PieceValues[PAWN] + PieceValues[BISHOP] - PieceValues[KNIGHT]);
    }

    {
        auto position = GameState::from_fen("1k2r3/8/8/3n2B1/4Pp2/8/3K4/8 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT), PieceValues[PAWN]);
    }

    // quiet moves (no capture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/8/8/8/8/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), 0);
    }

    // quiet moves (recapture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/5n2/8/8/8/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), -PieceValues[BISHOP]);
    }

    // quiet moves (complex recapture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/5n2/8/8/5P2/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), -PieceValues[BISHOP] + PieceValues[KNIGHT]);
    }
}
