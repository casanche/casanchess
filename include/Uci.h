#pragma once

#include "Board.h"
#include "Search.h"
#include "SearchLimits.h"

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
    ~Uci();
    void Bench(int depth, bool verbose);
    void Launch();

    // Search Outputs
    static void Output(int depth, int seldepth, int score, u64 nodes, i64 time, uint nps, int tbHits, BOUND_TYPE bound, const std::string& PV, const TT& tt);
    static void RootUpdate(int depth, int seldepth, int currMoveNumber, const std::string& currMove, u64 nodes, i64 time, uint nps, int tbHits);
    static void BestMove(const std::string& bestMove, const std::string& ponderMove);
    
private:
    void Go(std::istringstream &stream);
    void Position(std::istringstream &stream);
    void SetOption(std::istringstream &stream);
    
    void StartSearch();

    void ShowHashMoves();
    void StopAndJoin();

    UCI_Limits m_limits;
    
    TT m_tt;
    Search m_search;
    Board m_board;

    std::thread m_searchThread;
};
