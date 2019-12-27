#pragma once
#include "Position.h"

void LegalMoves(Position& position, std::vector<Move>& moves);
void QuiescenceMoves(Position& position, std::vector<Move>& moves);
void GeneratePsudoLegalMoves(const Position& position, std::vector<Move>& moves);

bool IsInCheck(const Position & position, unsigned int square, bool colour);


