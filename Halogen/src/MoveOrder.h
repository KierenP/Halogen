#pragma once
#include "Move.h"
#include "TranspositionTable.h"
#include "Position.h"


class MoveGenerator;
Move GetHashMove(const Position& position, int distanceFromRoot);