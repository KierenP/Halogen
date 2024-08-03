#include "History.h"
#include "BitBoardDefine.h"
#include "SearchData.h"

int16_t* ButterflyHistory::get(const GameState& position, const SearchStackState*, Move move)
{
    const auto& stm = position.Board().stm;
    return &table[stm][move.GetFrom()][move.GetTo()];
}
