#ifndef _SEARCH_H
#define _SEARCH_H

#include "Board.h"
#include "Move.h"
#include "Sorting.h"
#include "TT.h"

#include <chrono>
#include <vector>

const int MAX_ROOTMOVES = 250;

struct Limits;

struct RootMove {
    Move move;
    unsigned int nodeCount = 0;
};

class Search {
    typedef std::chrono::steady_clock Clock;
    
public:
    Search();
    Search(Board board);

    void IterativeDeepening(Board &board);
    inline void MakeMove(Board &board) { board.MakeMove(m_bestMove); }; //Interface

    //Flow
    void Stop() {
        m_stop = true;
    }

    //Helpers
    inline int64_t ElapsedTime() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_startTime).count();
    };

    //Set limits
    void AllocateLimits(Board &board, Limits limits);
    inline void FixDepth(int depth) {
        m_maxDepth = depth;
        m_allocatedTime = INFINITE;
    }
    inline void FixTime(int time) {
        m_allocatedTime = INFINITE;
        m_forcedTime = time;
    }
    inline void Infinite() {
        m_maxDepth = MAX_DEPTH;
        m_allocatedTime = INFINITE;
        m_forcedTime = INFINITE;
    }

    //Getters
    Move BestMove() const { return m_bestMove; };

    //Debug
    void ProbeBoard();

private:
    int QuiescenceSearch(Board &board, int alpha, int beta);
    int NegaMax(Board  &board, int depth, int alpha, int beta);
    int RootMax(Board &board, int depth, int alpha = -INFINITE_SCORE, int beta = INFINITE_SCORE);

    bool TimeOver();

    //IterativeDeepening methods
    void ClearSearch();
    void UciOutput(std::string PV);

    //Debug
    void ShowDebugInfo();

    //Limits
    int m_maxDepth;
    int m_allocatedTime;
    int m_forcedTime;
    U64 m_forcedNodes;

    //Fundamental variables
    Board m_initialBoard;
    TT m_tt;

    //Info variables
    int m_depth;
    int64_t m_elapsedTime;
    U64 m_nodes;
    int m_nps;
    int m_bestScore;
    Move m_bestMove;

    //Time management
    Clock::time_point m_startTime;
    bool m_stop;
    int m_nodesTimeCheck;

    //Helpers
    int m_ply;
    U8 m_counter;
    bool m_nullmoveAllowed;
    bool m_futility = false;

    //Heuristics
    Heuristics m_heuristics;

    //Root moves
    std::vector<RootMove> m_rootMoves;

    //Debug
    uint debug_rootMax = 0;
    uint debug_negaMax = 0;
    uint debug_quiescence = 0;
    uint debug_negaMaxRep = 0;
    uint debug_MDP = 0;
    uint debug_tthits = 0;
    uint debug_nullmove = 0;
    uint debug_negaMax_generation = 0;
    uint debug_negaMax_cutoffs = 0;
    uint debug_quiescence_tthits = 0;
};

struct Limits {
    bool infinite = false;
    int depth = 0;
    int nodes = 0;
    int moveTime = 0;

    int wtime;
    int btime;

    int winc;
    int binc;

    int movesToGo = 0;
};

#endif //_SEARCH_H
