#include "movegen/move.h"

#include <cassert>
#include <iostream>

std::ostream& operator<<(std::ostream& os, Move m)
{
    char buffer[6] = {};
    Square from = m.from();
    Square to = m.to();

    if (m.flag() == A_SIDE_CASTLE)
    {
        to = get_square(FILE_C, enum_to<Rank>(to));
    }
    else if (m.flag() == H_SIDE_CASTLE)
    {
        to = get_square(FILE_G, enum_to<Rank>(to));
    }

    buffer[0] = 'a' + enum_to<File>(from);
    buffer[1] = '1' + enum_to<Rank>(from);
    buffer[2] = 'a' + enum_to<File>(to);
    buffer[3] = '1' + enum_to<Rank>(to);

    if (m.is_promotion())
    {
        if (m.flag() == KNIGHT_PROMOTION || m.flag() == KNIGHT_PROMOTION_CAPTURE)
            buffer[4] = 'n';
        else if (m.flag() == BISHOP_PROMOTION || m.flag() == BISHOP_PROMOTION_CAPTURE)
            buffer[4] = 'b';
        else if (m.flag() == QUEEN_PROMOTION || m.flag() == QUEEN_PROMOTION_CAPTURE)
            buffer[4] = 'q';
        else if (m.flag() == ROOK_PROMOTION || m.flag() == ROOK_PROMOTION_CAPTURE)
            buffer[4] = 'r';
    }

    return os << buffer;
}

std::ostream& operator<<(std::ostream& os, format_chess960 f)
{
    const auto& m = f.m;
    char buffer[6] = {};
    Square from = m.from();
    Square to = m.to();

    buffer[0] = 'a' + enum_to<File>(from);
    buffer[1] = '1' + enum_to<Rank>(from);
    buffer[2] = 'a' + enum_to<File>(to);
    buffer[3] = '1' + enum_to<Rank>(to);

    if (f.m.is_promotion())
    {
        if (m.flag() == KNIGHT_PROMOTION || m.flag() == KNIGHT_PROMOTION_CAPTURE)
            buffer[4] = 'n';
        else if (m.flag() == BISHOP_PROMOTION || m.flag() == BISHOP_PROMOTION_CAPTURE)
            buffer[4] = 'b';
        else if (m.flag() == QUEEN_PROMOTION || m.flag() == QUEEN_PROMOTION_CAPTURE)
            buffer[4] = 'q';
        else if (m.flag() == ROOK_PROMOTION || m.flag() == ROOK_PROMOTION_CAPTURE)
            buffer[4] = 'r';
    }

    return os << buffer;
}
