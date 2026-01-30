#pragma once

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "movegen/move.h"

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string_view>

/*

This class contains the representation of a current state of the chessboard.
It does not store the history of all previous moves applied to the board.

*/

class BoardState
{
public:
    BoardState() noexcept;

    Square en_passant = N_SQUARES;
    int fifty_move_count = 0;
    int half_turn_count = 1;

    Side stm = N_SIDES;
    uint64_t castle_squares = EMPTY;

    // number of plys since last repetition
    std::optional<int> repetition;
    bool three_fold_rep = false;

    [[nodiscard]] Piece get_square_piece(Square square) const noexcept;

    [[nodiscard]] bool is_empty(Square square) const noexcept;
    [[nodiscard]] bool is_occupied(Square square) const noexcept;

    [[nodiscard]] uint64_t get_pieces_bb() const noexcept;
    [[nodiscard]] uint64_t get_empty_bb() const noexcept;
    [[nodiscard]] uint64_t get_pieces_bb(Side colour) const noexcept;
    [[nodiscard]] uint64_t get_pieces_bb(PieceType pieceType, Side colour) const noexcept;
    [[nodiscard]] uint64_t get_pieces_bb(Piece piece) const noexcept;
    [[nodiscard]] uint64_t get_pieces_bb(PieceType type) const noexcept;

    [[nodiscard]] Square get_king_sq(Side colour) const noexcept;

    void add_piece_sq(Square square, Piece piece) noexcept;
    void remove_piece_sq(Square square, Piece piece) noexcept;
    void clear_sq(Square square) noexcept;

    bool init_from_fen(const std::array<std::string_view, 6>& fen) noexcept;

    void apply_move(Move move) noexcept;
    void apply_null_move() noexcept;

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    [[nodiscard]] MoveFlag infer_move_flag(Square from, Square to) const noexcept;

    friend std::ostream& operator<<(std::ostream& os, const BoardState& b);

    uint64_t active_lesser_threats() const noexcept;

private:
    void recalculate_side_bb() noexcept;
    void update_castle_rights(Move move) noexcept;

    void update_lesser_threats() noexcept;
    void update_checkers() noexcept;
    void update_pinned() noexcept;

public:
    uint64_t key = 0;
    uint64_t pawn_key = 0;
    std::array<uint64_t, 2> non_pawn_key {};

    // mask of squares threatened by a lesser piece e.g lesser_threats[BISHOP] contains all squares attacked by enemy
    // pawns and knights
    std::array<uint64_t, N_PIECE_TYPES> lesser_threats {};

    uint64_t checkers {};
    uint64_t pinned {};

private:
    std::array<uint64_t, N_PIECE_TYPES> board = {};
    std::array<uint64_t, 2> side_bb = {};
    std::array<Piece, N_SQUARES> mailbox;
};