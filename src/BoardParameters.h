#pragma once
#include <memory>
#include <string>
#include <vector>

#include "BitBoardDefine.h"

class Move;
class Zobrist;

struct BoardParameterData
{
    Square m_EnPassant = N_SQUARES;
    unsigned int m_FiftyMoveCount = 0;
    unsigned int m_TurnCount = 1;

    Players m_CurrentTurn = N_PLAYERS;
    bool m_WhiteKingCastle = false;
    bool m_WhiteQueenCastle = false;
    bool m_BlackKingCastle = false;
    bool m_BlackQueenCastle = false;
};

class BoardParameters
{
public:
    BoardParameters() = default;
    virtual ~BoardParameters() = 0;

    BoardParameters(const BoardParameters&) = default;
    BoardParameters(BoardParameters&&) = default;
    BoardParameters& operator=(const BoardParameters&) = default;
    BoardParameters& operator=(BoardParameters&&) = default;

    unsigned int GetTurnCount() const { return Current->m_TurnCount; }
    Players GetTurn() const { return Current->m_CurrentTurn; }
    bool GetCanCastleWhiteKingside() const { return Current->m_WhiteKingCastle; }
    bool GetCanCastleWhiteQueenside() const { return Current->m_WhiteQueenCastle; }
    bool GetCanCastleBlackKingside() const { return Current->m_BlackKingCastle; }
    bool GetCanCastleBlackQueenside() const { return Current->m_BlackQueenCastle; }
    Square GetEnPassant() const { return Current->m_EnPassant; }
    unsigned int GetFiftyMoveCount() const { return Current->m_FiftyMoveCount; }

    void SetTurnCount(unsigned int val) { Current->m_TurnCount = val; }
    void SetTurn(Players val) { Current->m_CurrentTurn = val; }
    void SetCanCastleWhiteKingside(bool val) { Current->m_WhiteKingCastle = val; }
    void SetCanCastleWhiteQueenside(bool val) { Current->m_WhiteQueenCastle = val; }
    void SetCanCastleBlackKingside(bool val) { Current->m_BlackKingCastle = val; }
    void SetCanCastleBlackQueenside(bool val) { Current->m_BlackQueenCastle = val; }
    void SetEnPassant(Square val) { Current->m_EnPassant = val; }
    void SetFiftyMoveCount(unsigned int val) { Current->m_FiftyMoveCount = val; }

protected:
    bool InitialiseParametersFromFen(const std::vector<std::string>& fen);
    void SaveParameters();
    void RestorePreviousParameters();
    void UpdateCastleRights(Move move, Zobrist& zobrist_key);
    void NextTurn();
    void Increment50Move() { SetFiftyMoveCount(GetFiftyMoveCount() + 1); }
    void Reset50Move() { SetFiftyMoveCount(0); }

    //The only function like this, because we need to be able to do this when detecting 50 move repititions
    unsigned int GetPreviousFiftyMove(unsigned int index) const { return PreviousParameters.at(index).m_FiftyMoveCount; }

    void InitParameters();

private:
    std::vector<BoardParameterData> PreviousParameters = { BoardParameterData() };
    std::vector<BoardParameterData>::iterator Current = PreviousParameters.begin();
};
