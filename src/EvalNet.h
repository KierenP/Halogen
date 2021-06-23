#pragma once
#include "Position.h"
#include <functional>
#include <valarray>
#include <array>
#include <algorithm>

bool DeadPosition(const Position& position);
int EvaluatePositionNet(const Position& position, EvalCacheTable& evalTable);

namespace UnitTestEvalNet
{
	void NoPawnAdjustment(int& eval, const Position& position);
	void TempoAdjustment(int& eval, const Position& position);
}

