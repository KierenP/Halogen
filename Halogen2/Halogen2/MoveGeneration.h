#pragma once
#include "Position.h"

std::vector<Move> GenerateLegalMoves(Position& position);
std::vector<Move> GeneratePsudoLegalMoves(const Position& position);


