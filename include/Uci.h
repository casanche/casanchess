#pragma once

#include "Board.h"
#include "Search.h"

#include <sstream>
#include <thread>

inline bool UCI_PONDER = false;
inline bool UCI_CLASSICAL_EVAL = false;
inline uint UCI_SYZYGY_PROBE_LIMIT = 7;

inline bool UCI_OUTPUT = true;

constexpr int UCI_OUTPUT_ROOTMAX_MINTIME = 1000; //ms

class Uci {
public:
    Uci();
    void Bench(int depth, bool verbose);
    void Launch();

    // Search Outputs
    static void Output(int depth, int seldepth, int score, u64 nodes, i64 time, uint nps, int tbHits, BOUND_TYPE bound, const std::string& PV);
    static void RootUpdate(int depth, int seldepth, int currMoveNumber, const std::string& currMove, u64 nodes, i64 time, uint nps, int tbHits);
    static void BestMove(const std::string& bestMove, const std::string& ponderMove);

private:
    void Go(std::istringstream &stream);
    void Position(std::istringstream &stream);
    void SetOption(std::istringstream &stream);
    void StartSearch(UCI_Limits limits);
    void StopAndJoin();

    Board m_board;
    Search m_search;
    std::thread m_searchThread;
};
