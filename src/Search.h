#pragma once

#include "GameState.h"

class SearchSharedState;

void SearchThread(GameState& position, SearchSharedState& sharedData);