#pragma once

#include "BitBoardDefine.h"
#include "Move.h"
#include "Zobrist.h"

#include <vector>

class Move;
class Network;

/*

This class contains the representation of a current state of the chessboard.
It does not store the history of all previous moves applied to the board.

*/

class BoardState
{
public:
    BoardState();

    Square en_passant = N_SQUARES;
    unsigned int fifty_move_count;
    unsigned int half_turn_count;

    Players stm = N_PLAYERS;
    BB castle_squares;

    Pieces GetSquare(Square square) const;

    bool IsEmpty(Square square) const;
    bool IsOccupied(Square square) const;

    BB GetAllPieces() const;
    BB GetEmptySquares() const;
    BB GetPiecesColour(Players colour) const;
    BB GetPieceBB(PieceTypes pieceType, Players colour) const;
    BB GetPieceBB(Pieces piece) const;

    Square GetKing(Players colour) const;

    template <Players side>
    BB GetPieces() const
    {
        return side_bb[side];
    }

    template <PieceTypes type>
    BB GetPieceBB() const
    {
        return GetPieceBB<type, WHITE>() | GetPieceBB<type, BLACK>();
    }

    template <Pieces type>
    BB GetPieceBB() const
    {
        return board[type];
    }

    template <PieceTypes pieceType, Players colour>
    BB GetPieceBB() const
    {
        return GetPieceBB<Piece(pieceType, colour)>();
    }

    uint64_t GetZobristKey() const;
    uint64_t GetPawnKey() const;

    void AddPiece(Square square, Pieces piece);
    void RemovePiece(Square square, Pieces piece);
    void ClearSquare(Square square);

    void Reset();
    bool InitialiseFromFen(const std::array<std::string_view, 6>& fen);
    void UpdateCastleRights(Move move, Zobrist& zobrist_key);

    void ApplyMove(Move move);
    void ApplyNullMove();

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    MoveFlag GetMoveFlag(Square from, Square to) const;

    friend std::ostream& operator<<(std::ostream& os, const BoardState& b);

private:
    void AddPieceAndUpdate(Square square, Pieces piece);
    void RemovePieceAndUpdate(Square square, Pieces piece);
    void ClearSquareAndUpdate(Square square);
    void RecalculateWhiteBlackBoards();

    Zobrist key;
    PawnKey pawn_key;

    std::array<BB, N_PIECES> board = {};
    std::array<BB, 2> side_bb;
};