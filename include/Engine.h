#pragma once

#include "Board.h"
#include "Hash.h"
#include "Search.h"

struct Engine {
    Engine();

    void NewGame();
    void StartSearch(const UCI_Limits& limits);
    void StopSearch();

    TT tt;
    Search search;
    Board board;
};
