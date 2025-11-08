#pragma once

class GameState;
class SearchSharedState;
struct SearchLocalState;

void launch_worker_search(GameState& position, SearchLocalState& local, SearchSharedState& shared);