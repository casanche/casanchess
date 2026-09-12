#pragma once

#include "Hash.h"
#include "Search.h"
#include "SearchLimits.h"

#include <memory>
#include <sstream>
#include <thread>

struct Engine;

inline bool UCI_PONDER = false;
inline bool UCI_CLASSICAL_EVAL = false;
inline uint UCI_SYZYGY_PROBE_LIMIT = 7;
inline int UCI_DRAW_CONTEMPT = 10;
constexpr int UCI_AMBITION_DEFAULT = 0;
inline int UCI_AMBITION = UCI_AMBITION_DEFAULT; // Maximum root-relative static-eval adjustment in centipawns

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
    
    void ShowHashMoves();
    void StopAndJoin();
    
    std::unique_ptr<Engine> m_engine;
    std::thread m_searchThread;
};
