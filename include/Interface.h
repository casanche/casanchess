#ifndef INTERFACE_H
#define INTERFACE_H

#include "Board.h"
#include "MoveGenerator.h"

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

#endif //INTERFACE_H
