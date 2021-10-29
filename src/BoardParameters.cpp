#include "BoardParameters.h"
#include "Zobrist.h"

BoardParameters::~BoardParameters() = default;

void BoardParameters::NextTurn()
{
    SetTurnCount(GetTurnCount() + 1);
    SetTurn(!GetTurn());
}

void BoardParameters::InitParameters()
{
    PreviousParameters = { BoardParameterData() };
    Current = PreviousParameters.begin();
}

bool BoardParameters::InitialiseParametersFromFen(const std::vector<std::string>& fen)
{
    InitParameters();

    if (fen[1] == "w")
        SetTurn(WHITE);

    if (fen[1] == "b")
        SetTurn(BLACK);

    if (fen[2].find('K') != std::string::npos)
        SetCanCastleWhiteKingside(true);

    if (fen[2].find('Q') != std::string::npos)
        SetCanCastleWhiteQueenside(true);

    if (fen[2].find('k') != std::string::npos)
        SetCanCastleBlackKingside(true);

    if (fen[2].find('q') != std::string::npos)
        SetCanCastleBlackQueenside(true);

    SetEnPassant(static_cast<Square>(AlgebraicToPos(fen[3])));

    SetFiftyMoveCount(std::stoi(fen[4]));
    SetTurnCount(std::stoi(fen[5]));

    return true;
}

void BoardParameters::SaveParameters()
{
    PreviousParameters.emplace_back(*Current);
    Current = --PreviousParameters.end(); //Current might be invalidated by the above line if reallocation occurs
}

void BoardParameters::RestorePreviousParameters()
{
    assert(PreviousParameters.size() != 0);

    --Current;
    PreviousParameters.pop_back(); //the iterator should never be invalitated by this
}

void BoardParameters::UpdateCastleRights(Move move, uint64_t& zobrist_key)
{
    if (move.GetFrom() == SQ_E1 || move.GetTo() == SQ_E1) //Check for the piece moving off the square, or a capture happening on the square (enemy moving to square)
    {
        if (GetCanCastleWhiteKingside())
            zobrist_key ^= ZobristTable[12 * 64 + 1];

        if (GetCanCastleWhiteQueenside())
            zobrist_key ^= ZobristTable[12 * 64 + 2];

        SetCanCastleWhiteKingside(false);
        SetCanCastleWhiteQueenside(false);
    }

    if (move.GetFrom() == SQ_E8 || move.GetTo() == SQ_E8)
    {
        if (GetCanCastleBlackKingside())
            zobrist_key ^= ZobristTable[12 * 64 + 3];

        if (GetCanCastleBlackQueenside())
            zobrist_key ^= ZobristTable[12 * 64 + 4];

        SetCanCastleBlackKingside(false);
        SetCanCastleBlackQueenside(false);
    }

    if (move.GetFrom() == SQ_A1 || move.GetTo() == SQ_A1)
    {
        if (GetCanCastleWhiteQueenside())
            zobrist_key ^= ZobristTable[12 * 64 + 2];

        SetCanCastleWhiteQueenside(false);
    }

    if (move.GetFrom() == SQ_A8 || move.GetTo() == SQ_A8)
    {
        if (GetCanCastleBlackQueenside())
            zobrist_key ^= ZobristTable[12 * 64 + 4];

        SetCanCastleBlackQueenside(false);
    }

    if (move.GetFrom() == SQ_H1 || move.GetTo() == SQ_H1)
    {
        if (GetCanCastleWhiteKingside())
            zobrist_key ^= ZobristTable[12 * 64 + 1];

        SetCanCastleWhiteKingside(false);
    }

    if (move.GetFrom() == SQ_H8 || move.GetTo() == SQ_H8)
    {
        if (GetCanCastleBlackKingside())
            zobrist_key ^= ZobristTable[12 * 64 + 3];

        SetCanCastleBlackKingside(false);
    }
}
