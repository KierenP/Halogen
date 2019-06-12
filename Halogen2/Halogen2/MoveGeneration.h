#pragma once
#include "Position.h"

std::vector<Move> GenerateLegalMoves(Position& position);
std::vector<Move> GenerateLegalCaptures(Position& position);
std::vector<Move> GenerateLegalHangedCaptures(Position& position);						//captures of only undefended peices
void GeneratePsudoLegalMoves(const Position& position, std::vector<Move>& moves);

bool IsInCheck(const Position & position, unsigned int square, bool colour);


