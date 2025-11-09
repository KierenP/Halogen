#include "bitboard/enum.h"
#include "chessboard/game_state.h"
#include "movegen/move.h"
#include "search/score.h"
#include "search/static_exchange_evaluation.h"
#include "spsa/tuneable.h"

#include <array>
#include <cassert>
#include <string_view>

const auto test_see
    = []([[maybe_unused]] const GameState& position, [[maybe_unused]] Move move, [[maybe_unused]] Score expected_value)
{
    assert(see_ge(position.board(), move, expected_value));
    assert(!see_ge(position.board(), move, expected_value + 1));
};

void static_exchange_evaluation_test()
{
    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/4p3/1b2P3/2P4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), see_values[BISHOP]);
    }

    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/2p1p3/1b2P3/2P4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), see_values[BISHOP] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("rnbqk1nr/pp1p1ppp/8/2p1p3/1b2P3/2Q4P/PP1P1PP1/RNBQKBNR w KQkq - 0 4");
        test_see(position, Move(SQ_C3, SQ_B4, CAPTURE), see_values[BISHOP] - see_values[QUEEN]);
    }

    {
        auto position = GameState::from_fen("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -");
        test_see(position, Move(SQ_D3, SQ_E5, CAPTURE), see_values[PAWN] - see_values[KNIGHT]);
    }

    {
        auto exchange1 = see_values[QUEEN] - see_values[ROOK];
        auto exchange2 = exchange1 + see_values[BISHOP] - see_values[ROOK] + see_values[BISHOP];
        auto position = GameState::from_fen("1k1r4/1pp4p/p2b1b2/4q3/8/P3R1P1/1PP1R1BP/2K1Q3 w - - 0 1");
        test_see(position, Move(SQ_E3, SQ_E5, CAPTURE), std::max(exchange1, exchange2));
    }

    // en-passant tests, with discovered attacks

    {
        const auto exchange1 = see_values[PAWN];
        const auto exchange2 = exchange1 - see_values[PAWN] + see_values[BISHOP] - see_values[KNIGHT];
        auto position = GameState::from_fen("1k6/8/4R3/6B1/2n1Pp2/8/8/1K6 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT), std::min(exchange1, exchange2));
    }

    // king attacks

    {
        auto position = GameState::from_fen("1k6/4r3/8/8/8/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E7, SQ_E3, CAPTURE), see_values[PAWN] - see_values[ROOK]);
    }

    {
        auto position = GameState::from_fen("1k2r3/4r3/8/8/8/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E7, SQ_E3, CAPTURE), see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("4r3/8/8/8/3k4/4P3/3K4/8 b - - 0 1");
        test_see(position, Move(SQ_E8, SQ_E3, CAPTURE), see_values[PAWN]);
    }

    // king attacks + en passant + discovered attacks

    {
        auto exchange1 = see_values[PAWN];
        auto exchange2 = exchange1 - see_values[PAWN] + see_values[BISHOP] - see_values[KNIGHT];
        auto position = GameState::from_fen("1k6/8/8/3n2B1/4Pp2/8/3K4/8 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT), std::min(exchange1, exchange2));
    }

    {
        auto position = GameState::from_fen("1k2r3/8/8/3n2B1/4Pp2/8/3K4/8 b - e3 0 1");
        test_see(position, Move(SQ_F4, SQ_E3, EN_PASSANT), see_values[PAWN]);
    }

    // quiet moves (no capture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/8/8/8/8/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), 0);
    }

    // quiet moves (recapture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/5n2/8/8/8/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), -see_values[BISHOP]);
    }

    // quiet moves (complex recapture)

    {
        auto position = GameState::from_fen("3k4/8/1B6/5n2/8/8/5P2/3K4 w - - 0 1");
        test_see(position, Move(SQ_B6, SQ_E3, QUIET), std::min(0, -see_values[BISHOP] + see_values[KNIGHT]));
    }

    // promotion tests

    {
        auto position = GameState::from_fen("8/7P/8/8/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_H8, QUEEN_PROMOTION), see_values[QUEEN] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("3r4/7P/8/8/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_H8, QUEEN_PROMOTION), -see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("3r4/7P/8/7R/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_H8, KNIGHT_PROMOTION), see_values[KNIGHT] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("3r4/7P/8/7R/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_H8, QUEEN_PROMOTION), see_values[ROOK] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("6n1/7P/8/8/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_G8, ROOK_PROMOTION_CAPTURE),
            see_values[ROOK] - see_values[PAWN] + see_values[KNIGHT]);
    }

    {
        auto position = GameState::from_fen("3r2n1/7P/8/8/8/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_G8, KNIGHT_PROMOTION_CAPTURE), see_values[KNIGHT] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("3r2n1/7P/8/8/6R1/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_G8, BISHOP_PROMOTION_CAPTURE),
            see_values[KNIGHT] + see_values[BISHOP] - see_values[PAWN]);
    }

    {
        auto position = GameState::from_fen("3r2n1/7P/8/8/6R1/k7/8/K7 w - - 0 1");
        test_see(position, Move(SQ_H7, SQ_G8, QUEEN_PROMOTION_CAPTURE),
            see_values[KNIGHT] - see_values[PAWN] + see_values[ROOK]);
    }

    // Castle moves always have zero SEE

    {
        auto position = GameState::from_fen("rnbqkbnr/pp3ppp/2p5/3pp3/8/4PN2/PPPPBPPP/RNBQK2R w KQkq - 0 4");
        test_see(position, Move(SQ_E1, SQ_H1, H_SIDE_CASTLE), 0);
    }
}
