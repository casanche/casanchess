#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Hash.h"
#include "Move.h"
#include "Sorting.h"

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

    class Debug {
    public:
        void Increment(std::string theVariable) { debugVariables[theVariable]++; };
        void Print();
    private:
        void Transform();
        std::map<std::string, int> debugVariables;
    };
    
public:
    Search();
    Search(Board board);

    void IterativeDeepening(Board &board);
    inline void MakeMove(Board &board) { board.MakeMove(m_bestMove); }; //Interface

    //Flow
    void Stop() { m_stop = true; }
    void DebugMode() { m_debugMode = true; }

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
    void ProbeBoard(Board& board);

private:
    int QuiescenceSearch(Board &board, int alpha, int beta);
    int NegaMax(Board  &board, int depth, int alpha, int beta);
    int RootMax(Board &board, int depth, int alpha, int beta);

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
    u64 m_forcedNodes;

    //Info variables
    int m_depth;
    int64_t m_elapsedTime;
    u64 m_nodes;
    int m_nps;
    int m_bestScore;
    Move m_bestMove;

    //Time management
    Clock::time_point m_startTime;
    bool m_stop;
    int m_nodesTimeCheck;

    //Helpers
    int m_ply;
    int m_selPly;
    u8 m_counter;
    bool m_nullmoveAllowed;

    //Heuristics
    Heuristics m_heuristics;

    //Root moves
    std::vector<RootMove> m_rootMoves;

    //Debug
    bool m_debugMode;
    Debug m_debug;
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

#endif //SEARCH_H
