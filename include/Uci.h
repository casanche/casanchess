#ifndef _UCI_H
#define _UCI_H

#include "Board.h"
#include "Search.h"

#include <sstream>

class Uci {
public:
    Uci();
    void Launch();

private:
    void Go(std::istringstream &stream);
    void Position(std::istringstream &stream);

    Board m_board;
    Search m_search;
};

#endif //_UCI_H