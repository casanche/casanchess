#pragma once

#include "Board.h"
#include "Hash.h"
#include "Search.h"

#include <string>

class Interface {
public:
    Interface();
    void NewGame();
    void Print();
    void Start(std::string fenString = "");
private:
    void PrintWelcome();

    TT m_tt;
    Search m_search;
    Board m_board;
};
