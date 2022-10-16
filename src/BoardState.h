#pragma once

#include "BitBoardDefine.h"
#include "Move.h"
#include "Zobrist.h"
#include "td-leaf/HalogenNetwork.h"

#include <vector>

class Move;
class HalogenNetwork;

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
    uint64_t castle_squares;

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
        if constexpr (side == Players::WHITE)
        {
            return WhitePieces;
        }
        else
        {
            return BlackPieces;
        }
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

    void Print() const;

    void SetSquare(Square square, Pieces piece);
    void ClearSquare(Square square);

    void Reset();
    bool InitialiseFromFen(const std::vector<std::string>& fen);
    void UpdateCastleRights(Move move, Zobrist& zobrist_key);

    void ApplyMove(Move move, HalogenNetwork& net);
    void ApplyNullMove();

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    MoveFlag GetMoveFlag(Square from, Square to) const;

private:
    void SetSquareAndUpdate(Square square, Pieces piece, HalogenNetwork& net);
    void ClearSquareAndUpdate(Square square, HalogenNetwork& net);

    // optimization: GetWhitePieces/GetBlackPieces can return a precalculated bitboard
    // which is updated only when needed
    void RecalculateWhiteBlackBoards();
    uint64_t WhitePieces;
    uint64_t BlackPieces;

    Zobrist key;

    std::array<uint64_t, N_PIECES> board = {};
};