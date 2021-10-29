#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"

Position::Position()
{
    StartingPosition();
    zobrist.Recalculate(*this);
}

void Position::ApplyMove(Move move)
{
    PreviousKeys.push_back(zobrist);
    moveStack.push_back(move);
    net.DoMove();
    SaveParameters();
    SaveBoard();

    zobrist.ToggleSTM();

    // undo the previous ep square
    if (GetEnPassant() <= SQ_H8)
        zobrist.ToggleEnpassant(GetFile(GetEnPassant()));

    SetEnPassant(N_SQUARES);
    Increment50Move();

    if (move.IsCapture() && move.GetFlag() != EN_PASSANT)
    {
        net.UpdateInput<Network::Toggle::Remove>(move.GetTo(), GetSquare(move.GetTo()));
        zobrist.TogglePieceSquare(GetSquare(move.GetTo()), move.GetTo());
    }

    if (!move.IsPromotion())
        SetSquareAndUpdate(move.GetTo(), GetSquare(move.GetFrom()));

    if (move.GetFlag() == KING_CASTLE)
    {
        SetSquareAndUpdate(GetPosition(FILE_F, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_H, GetRank(move.GetFrom()))));
        ClearSquareAndUpdate(GetPosition(FILE_H, GetRank(move.GetFrom())));
    }

    if (move.GetFlag() == QUEEN_CASTLE)
    {
        SetSquareAndUpdate(GetPosition(FILE_D, GetRank(move.GetFrom())), GetSquare(GetPosition(FILE_A, GetRank(move.GetFrom()))));
        ClearSquareAndUpdate(GetPosition(FILE_A, GetRank(move.GetFrom())));
    }

    //for some special moves we need to do other things
    switch (move.GetFlag())
    {
    case PAWN_DOUBLE_MOVE:
    {
        //average of from and to is the one in the middle, or 1 behind where it is moving to. This means it works the same for black and white
        Square ep = static_cast<Square>((move.GetTo() + move.GetFrom()) / 2);

        //it only counts as a ep square if a pawn could potentially do the capture!
        uint64_t potentialAttackers = PawnAttacks[GetTurn()][ep] & GetPieceBB(PAWN, !GetTurn());

        while (potentialAttackers)
        {
            Square sq = static_cast<Square>(LSBpop(potentialAttackers));

            ApplyMoveQuick({ sq, ep, EN_PASSANT });
            bool legal = !IsInCheck(*this, !GetTurn());
            RevertMoveQuick();

            if (legal)
            {
                SetEnPassant(ep);
                zobrist.ToggleEnpassant(GetFile(ep));
                break;
            }
        }

        break;
    }
    case EN_PASSANT:
        ClearSquareAndUpdate(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
        break;
    case KNIGHT_PROMOTION:
    case KNIGHT_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(KNIGHT, GetTurn()));
        break;
    case BISHOP_PROMOTION:
    case BISHOP_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(BISHOP, GetTurn()));
        break;
    case ROOK_PROMOTION:
    case ROOK_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(ROOK, GetTurn()));
        break;
    case QUEEN_PROMOTION:
    case QUEEN_PROMOTION_CAPTURE:
        SetSquareAndUpdate(move.GetTo(), Piece(QUEEN, GetTurn()));
        break;
    default:
        break;
    }

    if (move.IsCapture() || GetSquare(move.GetTo()) == Piece(PAWN, GetTurn()) || move.IsPromotion())
        Reset50Move();

    ClearSquareAndUpdate(move.GetFrom());
    NextTurn();
    UpdateCastleRights(move, zobrist);

    assert(zobrist.Verify(*this));
}

void Position::ApplyMove(const std::string& strmove)
{
    Square prev = static_cast<Square>((strmove[0] - 97) + (strmove[1] - 49) * 8);
    Square next = static_cast<Square>((strmove[2] - 97) + (strmove[3] - 49) * 8);
    MoveFlag flag = QUIET;

    //Captures
    if (IsOccupied(next))
        flag = CAPTURE;

    //Double pawn moves
    if (AbsRankDiff(prev, next) == 2)
        if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
            flag = PAWN_DOUBLE_MOVE;

    //En passant
    if (next == GetEnPassant())
        if (GetSquare(prev) == WHITE_PAWN || GetSquare(prev) == BLACK_PAWN)
            flag = EN_PASSANT;

    //Castling
    if (prev == SQ_E1 && next == SQ_G1 && GetSquare(prev) == WHITE_KING)
        flag = KING_CASTLE;

    if (prev == SQ_E1 && next == SQ_C1 && GetSquare(prev) == WHITE_KING)
        flag = QUEEN_CASTLE;

    if (prev == SQ_E8 && next == SQ_G8 && GetSquare(prev) == BLACK_KING)
        flag = KING_CASTLE;

    if (prev == SQ_E8 && next == SQ_C8 && GetSquare(prev) == BLACK_KING)
        flag = QUEEN_CASTLE;

    //Promotion
    if (strmove.length() == 5) //promotion: c7c8q or d2d1n	etc.
    {
        if (tolower(strmove[4]) == 'n')
            flag = KNIGHT_PROMOTION;
        if (tolower(strmove[4]) == 'r')
            flag = ROOK_PROMOTION;
        if (tolower(strmove[4]) == 'q')
            flag = QUEEN_PROMOTION;
        if (tolower(strmove[4]) == 'b')
            flag = BISHOP_PROMOTION;

        if (IsOccupied(next)) //if it's a capture we need to shift the flag up 4 to turn it from eg: KNIGHT_PROMOTION to KNIGHT_PROMOTION_CAPTURE. EDIT: flag == capture wont work because we just changed the flag!! This was a bug back from 2018 found in 2020
            flag = static_cast<MoveFlag>(flag + CAPTURE); //might be slow, but don't care.
    }

    ApplyMove(Move(prev, next, flag));
    net.RecalculateIncremental(GetInputLayer());
}

void Position::RevertMove()
{
    assert(PreviousKeys.size() > 0);

    RestorePreviousBoard();
    RestorePreviousParameters();
    zobrist = PreviousKeys.back();
    PreviousKeys.pop_back();
    net.UndoMove();
    moveStack.pop_back();
}

void Position::ApplyNullMove()
{
    PreviousKeys.push_back(zobrist);
    moveStack.push_back(Move::Uninitialized);
    SaveParameters();

    zobrist.ToggleSTM();

    // undo the previous ep square
    if (GetEnPassant() <= SQ_H8)
        zobrist.ToggleEnpassant(GetFile(GetEnPassant()));

    SetEnPassant(N_SQUARES);
    Increment50Move();
    NextTurn();

    assert(zobrist.Verify(*this));
}

void Position::RevertNullMove()
{
    assert(PreviousKeys.size() > 0);

    RestorePreviousParameters();
    zobrist = PreviousKeys.back();
    PreviousKeys.pop_back();
    moveStack.pop_back();
}

void Position::Print() const
{
    std::cout << "\n  A B C D E F G H";

    char Letter[N_SQUARES];

    for (int i = 0; i < N_SQUARES; i++)
    {
        Letter[i] = PieceToChar(GetSquare(static_cast<Square>(i)));
    }

    for (int i = 0; i < N_SQUARES; i++)
    {
        unsigned int square = GetPosition(GetFile(i), 7 - GetRank(i)); //7- to flip on the y axis and do rank 8, 7 ...

        if (GetFile(square) == FILE_A)
        {
            std::cout << std::endl; //Go to a new line
            std::cout << 8 - GetRank(i); //Count down from 8
        }

        std::cout << " ";
        std::cout << Letter[square];
    }

    std::cout << std::endl;
}

void Position::StartingPosition()
{
    InitialiseFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
    net.RecalculateIncremental(GetInputLayer());
}

bool Position::InitialiseFromFen(std::vector<std::string> fen)
{
    if (fen.size() == 4)
    {
        fen.push_back("0");
        fen.push_back("1");
    }

    if (fen.size() < 6)
        return false; //bad fen

    if (!InitialiseBoardFromFen(fen))
        return false;

    if (!InitialiseParametersFromFen(fen))
        return false;

    zobrist.Recalculate(*this);
    net.RecalculateIncremental(GetInputLayer());

    return true;
}

bool Position::InitialiseFromFen(const std::string& board, const std::string& turn, const std::string& castle, const std::string& ep, const std::string& fiftyMove, const std::string& turnCount)
{
    std::vector<std::string> splitFen;
    splitFen.push_back(board);
    splitFen.push_back(turn);
    splitFen.push_back(castle);
    splitFen.push_back(ep);
    splitFen.push_back(fiftyMove);
    splitFen.push_back(turnCount);

    return InitialiseFromFen(splitFen);
}

bool Position::InitialiseFromFen(std::string fen)
{
    std::vector<std::string> splitFen; //Split the line into an array of strings seperated by each space
    std::istringstream iss(fen);
    splitFen.push_back("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    splitFen.push_back("w");
    splitFen.push_back("-");
    splitFen.push_back("-");
    splitFen.push_back("0");
    splitFen.push_back("1");

    for (int i = 0; i < 6 && iss; i++)
    {
        std::string stub;
        iss >> stub;
        if (!stub.empty())
            splitFen[i] = (stub);
    }

    return InitialiseFromFen(splitFen[0], splitFen[1], splitFen[2], splitFen[3], splitFen[4], splitFen[5]);
}

uint64_t Position::GetZobristKey() const
{
    return zobrist.Key();
}

void Position::Reset()
{
    PreviousKeys.clear();
    moveStack.clear();
    ResetBoard();
    InitParameters();
    zobrist.Recalculate(*this);
}

std::array<int16_t, INPUT_NEURONS> Position::GetInputLayer() const
{
    std::array<int16_t, INPUT_NEURONS> ret;

    for (int i = 0; i < N_PIECES; i++)
    {
        uint64_t bb = GetPieceBB(static_cast<Pieces>(i));

        for (int sq = 0; sq < N_SQUARES; sq++)
        {
            ret[i * 64 + sq] = ((bb & SquareBB[sq]) != 0);
        }
    }

    return ret;
}

void Position::ApplyMoveQuick(Move move)
{
    SaveBoard();

    SetSquare(move.GetTo(), GetSquare(move.GetFrom()));
    ClearSquare(move.GetFrom());

    if (move.GetFlag() == EN_PASSANT)
        ClearSquare(GetPosition(GetFile(move.GetTo()), GetRank(move.GetFrom())));
}

void Position::RevertMoveQuick()
{
    RestorePreviousBoard();
}

int16_t Position::GetEvaluation() const
{
    return net.Eval();
}

bool Position::CheckForRep(int distanceFromRoot, int maxReps) const
{
    int totalRep = 1;
    uint64_t current = GetZobristKey();

    //note Previous keys will not contain the current key, hence rep starts at one
    for (int i = static_cast<int>(PreviousKeys.size()) - 2; i >= 0; i -= 2)
    {
        if (PreviousKeys[i].Key() == current)
            totalRep++;

        if (totalRep == maxReps)
            return true; //maxReps (usually 3) reps is always a draw
        if (totalRep == 2 && static_cast<int>(PreviousKeys.size() - i) < distanceFromRoot - 1)
            return true; //Don't allow 2 reps if its in the local search history (not part of the actual played game)

        if (GetPreviousFiftyMove(i) == 0)
            break;
    }

    return false;
}

Move Position::GetPreviousMove() const
{
    if (moveStack.size())
        return moveStack.back();
    else
        return Move::Uninitialized;
}

void Position::SetSquareAndUpdate(Square square, Pieces piece)
{
    net.UpdateInput<Network::Toggle::Add>(square, piece);
    zobrist.TogglePieceSquare(piece, square);
    SetSquare(square, piece);
}

void Position::ClearSquareAndUpdate(Square square)
{
    Pieces piece = GetSquare(square);
    net.UpdateInput<Network::Toggle::Remove>(square, piece);
    zobrist.TogglePieceSquare(piece, square);
    ClearSquare(square);
}
