#include "Engine.h"

Engine::Engine() :
    tt(),
    search(tt),
    board()
{}

void Engine::NewGame() {
    tt.Clear();
    search.ClearSearch(true);
    board.Init();
}

void Engine::StartSearch(const UCI_Limits& limits) {
    search.IterativeDeepening(board, limits);
}

void Engine::StopSearch() {
    search.Stop();
}
