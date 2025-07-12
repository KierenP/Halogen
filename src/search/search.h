#pragma once

#include "search/thread.h"

class GameState;
class SearchSharedState;

void launch_worker_search(GameState& position, SearchLocalState& local, SearchSharedState& shared);