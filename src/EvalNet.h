#pragma once
#include "Position.h"
#include <algorithm>
#include <array>
#include <functional>
#include <valarray>

bool DeadPosition(const Position& position);
int EvaluatePositionNet(const Position& position, EvalCacheTable& evalTable);
