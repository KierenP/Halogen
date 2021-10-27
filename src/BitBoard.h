#pragma once
#include "BitBoardDefine.h"
#include <vector>

using BitBoardData = std::array<uint64_t, N_PIECES>;

class BitBoard
{
public:
    BitBoard() = default;
    virtual ~BitBoard() = 0;

    BitBoard(const BitBoard&) = default;
    BitBoard(BitBoard&&) = default;
    BitBoard& operator=(const BitBoard&) = default;
    BitBoard& operator=(BitBoard&&) = default;

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

    void SetSquare(Square square, Pieces piece);
    void ClearSquare(Square square);

protected:
    void ResetBoard();
    bool InitialiseBoardFromFen(const std::vector<std::string>& fen);

    void SaveBoard();
    void RestorePreviousBoard();

private:
    void RecalculateWhiteBlackBoards();

    uint64_t WhitePieces;
    uint64_t BlackPieces;

    std::vector<BitBoardData> previousBoards = { BitBoardData() };
};
