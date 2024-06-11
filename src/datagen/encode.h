#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "../BoardState.h"
#include "../Score.h"

struct MarlinFormat
{
    uint64_t occ; // 64-bit occupied-piece bitboard
    std::array<uint8_t, 16> pcs; // 32-entry array of 4-bit pieces
    uint8_t stm_ep; // 8-bit side-to-move and en-passant field
    uint8_t half_move_clock; // 8-bit halfmove clock
    uint16_t full_move_counter; // 16-bit fullmove counter
    int16_t score; // Score for the position
    uint8_t result; // Game result: black win is 0, draw is 1, white win is 2
    uint8_t padding; // Unused extra byte
};

static_assert(sizeof(MarlinFormat) == 32, "MarlinFormat size must be 32 bytes");
static_assert(std::is_standard_layout_v<MarlinFormat>);

/// - Result is 0.0 for Black Win, 0.5 for Draw, 1.0 for White Win
MarlinFormat convert(BoardState board, float result);
uint16_t convert(Move move);
int16_t convert(Players stm, Score score);