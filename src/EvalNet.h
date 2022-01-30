#pragma once

class EvalCacheTable;
class Position;

bool DeadPosition(const Position& position);
int EvaluatePositionNet(const Position& position, EvalCacheTable& evalTable);
