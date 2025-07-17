#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string_view>

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "movegen/move.h"
#include "search/zobrist.h"

/*

This class contains the representation of a current state of the chessboard.
It does not store the history of all previous moves applied to the board.

*/

class BoardState
{
public:
    BoardState();

    Square en_passant = N_SQUARES;
    int fifty_move_count = 0;
    int half_turn_count = 1;

    Side stm = N_SIDES;
    uint64_t castle_squares = EMPTY;

    // number of plys since last repetition
    std::optional<int> repetition;
    bool three_fold_rep = false;

    [[nodiscard]] Piece get_square_piece(Square square) const;

    [[nodiscard]] bool is_empty(Square square) const;
    [[nodiscard]] bool is_occupied(Square square) const;

    [[nodiscard]] uint64_t get_pieces_bb() const;
    [[nodiscard]] uint64_t get_empty_bb() const;
    [[nodiscard]] uint64_t get_pieces_bb(Side colour) const;
    [[nodiscard]] uint64_t get_pieces_bb(PieceType pieceType, Side colour) const;
    [[nodiscard]] uint64_t get_pieces_bb(Piece piece) const;
    [[nodiscard]] uint64_t get_pieces_bb(PieceType type) const;

    [[nodiscard]] Square get_king_sq(Side colour) const;

    void add_piece_sq(Square square, Piece piece);
    void remove_piece_sq(Square square, Piece piece);
    void clear_sq(Square square);

    bool init_from_fen(const std::array<std::string_view, 6>& fen);

    void apply_move(Move move);
    void apply_null_move();

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    [[nodiscard]] MoveFlag infer_move_flag(Square from, Square to) const;

    friend std::ostream& operator<<(std::ostream& os, const BoardState& b);

private:
    void recalculate_side_bb();
    void update_castle_rights(Move move);

public:
    uint64_t key = 0;
    uint64_t pawn_key = 0;
    std::array<uint64_t, 2> non_pawn_key {};
    // uint64_t minor_key = 0;
    // uint64_t major_key = 0;
    std::array<uint64_t, N_SIDES> pinned_pieces = {};

private:
    std::array<uint64_t, N_PIECES> board = {};
    std::array<uint64_t, 2> side_bb = {};
    std::array<Piece, N_SQUARES> mailbox;
};