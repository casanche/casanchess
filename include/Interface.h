#pragma once

#include "Board.h"

#include <string>

class Interface {
public:
    Interface();
    void NewGame();
    void Print();
    void Start(std::string fenString = "");
private:
    void PrintWelcome();
    Board m_board;
};
