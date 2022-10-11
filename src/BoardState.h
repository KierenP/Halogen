#pragma once

#include "BitBoardDefine.h"
#include "Zobrist.h"

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
    unsigned int turn_count; // BUG: is this the half move or full move count?

    Players stm = N_PLAYERS;
    bool white_king_castle;
    bool white_queen_castle;
    bool black_king_castle;
    bool black_queen_castle;

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
    uint64_t GetPieceBB() const { return GetPieceBB(type, WHITE) | GetPieceBB(type, BLACK); }

    uint64_t GetZobristKey() const;

    void Print() const;

    void SetSquare(Square square, Pieces piece);
    void ClearSquare(Square square);

    void Reset();
    bool InitialiseFromFen(const std::vector<std::string>& fen);
    void UpdateCastleRights(Move move, Zobrist& zobrist_key);

    void ApplyMove(Move move, Network& net);
    void ApplyNullMove();

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