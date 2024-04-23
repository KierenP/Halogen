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
    uint64_t castle_squares;

    Pieces GetSquare(Square square) const;

    bool IsEmpty(Square square) const;
    bool IsOccupied(Square square) const;
    bool IsOccupied(Square square, Players colour) const;

    uint64_t GetAllPieces() const;
    uint64_t GetEmptySquares() const;
    uint64_t GetWhitePieces() const;
    uint64_t GetBlackPieces() const;
    uint64_t GetPiecesColour(Players colour) const;
    uint64_t GetPieceBB(PieceTypes pieceType, Players colour) const;
    uint64_t GetPieceBB(Pieces piece) const;

    Square GetKing(Players colour) const;

    template <PieceTypes type>
    uint64_t GetPieceBB() const
    {
        return GetPieceBB(type, WHITE) | GetPieceBB(type, BLACK);
    }

    uint64_t GetZobristKey() const;

    void Print() const;

    void SetSquare(Square square, Pieces piece);
    void ClearSquare(Square square);

    void Reset();
    bool InitialiseFromFen(const std::vector<std::string>& fen);
    void UpdateCastleRights(Move move, Zobrist& zobrist_key);

    void ApplyMove(Move move, Network& net);
    void ApplyNullMove();

    // given a from/to square, infer which MoveFlag matches the current position (ignoring promotions)
    MoveFlag GetMoveFlag(Square from, Square to) const;

private:
    void SetSquareAndUpdate(Square square, Pieces piece, Network& net);
    void ClearSquareAndUpdate(Square square, Network& net);

    // optimization: GetWhitePieces/GetBlackPieces can return a precalculated bitboard
    // which is updated only when needed
    void RecalculateWhiteBlackBoards();
    uint64_t WhitePieces;
    uint64_t BlackPieces;

    Zobrist key;

    std::array<uint64_t, N_PIECES> board = {};
};