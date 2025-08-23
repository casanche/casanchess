#pragma once

#include "Board.h"
#include "Search.h"

#include <sstream>

inline bool UCI_PONDER = false;
inline bool UCI_CLASSICAL_EVAL = false;
inline uint UCI_SYZYGY_PROBE_LIMIT = 7;

inline bool UCI_OUTPUT = true;

class Uci {
public:
    Uci();
    void Bench(int depth, bool verbose);
    void Launch();

private:
    void Go(std::istringstream &stream);
    void Position(std::istringstream &stream);
    void SetOption(std::istringstream &stream);
    void StartSearch();

    Board m_board;
    Search m_search;
};
