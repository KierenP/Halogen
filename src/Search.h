#pragma once

class GameState;
class SearchSharedState;

void SearchThread(GameState& position, SearchSharedState& sharedData);