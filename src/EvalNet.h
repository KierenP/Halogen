#pragma once
#include "Position.h"
#include "EvalCache.h"
#include <functional>
#include <valarray>
#include <array>
#include <algorithm>

bool DeadPosition(const Position& position);
int EvaluatePositionNet(const Position& position, EvalCacheTable& evalTable);
