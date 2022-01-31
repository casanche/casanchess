#ifndef UCI_H
#define UCI_H

#include "Board.h"
#include "Search.h"

#include <sstream>

extern bool UCI_PONDER;
extern bool UCI_OUTPUT;

class Uci {
public:
    Uci();
    void Launch();

private:
    void Go(std::istringstream &stream);
    void Position(std::istringstream &stream);
    void SetOption(std::istringstream &stream);
    void StartSearch();

    Board m_board;
    Search m_search;
};

#endif //UCI_H