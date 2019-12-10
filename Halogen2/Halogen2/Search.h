#pragma once
#include "TranspositionTable.h"
#include "ABnode.h"
#include "Position.h"
#include "MoveGeneration.h"
#include "Zobrist.h"
#include "Evaluate.h"
#include "Move.h"
#include "TimeManage.h"
#include <ctime>
#include <algorithm>
#include <utility>

extern TranspositionTable tTable;
Move SearchPosition(Position position, int allowedTimeMs);
