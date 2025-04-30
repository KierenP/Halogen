#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string_view>

#include "BitBoardDefine.h"
#include "Move.h"
#include "Zobrist.h"

/*

This class contains the representation of a current state of the chessboard.
It does not store the history of all previous moves applied to the board.

*/

class BoardState
{
public:
    Square en_passant = N_SQUARES;
    int fifty_move_count = 0;
    int half_turn_count = 1;

    Players stm = N_PLAYERS;
    uint64_t castle_squares = EMPTY;

    // number of plys since last repetition
    std::optional<int> repetition;
    bool three_fold_rep = false;

    Pieces GetSquare(Square square) const;

    bool IsEmpty(Square square) const;
    bool IsOccupied(Square square) const;

    uint64_t GetAllPieces() const;
    uint64_t GetEmptySquares() const;
    uint64_t GetPiecesColour(Players colour) const;
    uint64_t GetPieceBB(PieceTypes pieceType, Players colour) const;
    uint64_t GetPieceBB(Pieces piece) const;

    Square GetKing(Players colour) const;

    template <Players side>
    uint64_t GetPieces() const
    {
        return side_bb[side];
    }

    template <PieceTypes type>
    uint64_t GetPieceBB() const
    {
        return GetPieceBB<type, WHITE>() | GetPieceBB<type, BLACK>();
    }

    template <Pieces type>
    uint64_t GetPieceBB() const
    {
        return board[type];
    }

    template <PieceTypes pieceType, Players colour>
    uint64_t GetPieceBB() const
    {
        return GetPieceBB<Piece(pieceType, colour)>();
    }

    uint64_t GetZobristKey() const;
    uint64_t GetPawnKey() const;

    void AddPiece(Square square, Pieces piece);
    void RemovePiece(Square square, Pieces piece);
    void ClearSquare(Square square);

    bool InitialiseFromFen(const std::array<std::string_view, 6>& fen);
    void UpdateCastleRights(Move move);

    void ApplyMove(Move move);
    void ApplyNullMove();

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    MoveFlag GetMoveFlag(Square from, Square to) const;

    friend std::ostream& operator<<(std::ostream& os, const BoardState& b);

private:
    void RecalculateWhiteBlackBoards();

    uint64_t key = 0;
    uint64_t pawn_key = 0;
    std::array<uint64_t, 2> non_pawn_key {};
    uint64_t minor_key = 0;
    uint64_t major_key = 0;

    std::array<uint64_t, N_PIECES> board = {};
    std::array<uint64_t, 2> side_bb = {};
};