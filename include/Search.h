#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Hash.h"
#include "Move.h"
#include "Sorting.h"
#include "Utils.h"

#include <vector>

const int MAX_ROOTMOVES = 250;

struct Limits {
    bool infinite = false;
    bool ponderhit = false;
    int depth = 0;
    int nodes = 0;
    int moveTime = 0;

    int wtime;
    int btime;

    int winc;
    int binc;

    int movesToGo = 0;
};

struct RootMove {
    Move move;
    unsigned int nodeCount = 0;
};

class Search {

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

    //Interface
    inline void MakeMove(Board &board) { board.MakeMove(m_bestMove); };

    //Flow
    int64_t ElapsedTime() { return m_clock.Elapsed(); }
    void Stop() { m_stop = true; }
    void DebugMode() { m_debugMode = true; }

    //Set limits
    Limits GetLimits() { return m_limits; }
    void AllocateLimits(Board &board, Limits limits);
    inline void FixDepth(int depth) {
        m_allocatedTime = INFINITE;
        m_maxDepth = depth;
    }
    //In milliseconds
    inline void FixTime(int time) {
        m_allocatedTime = INFINITE;
        m_forcedTime = time;
    }
    inline void FixNodes(int nodes) {
        m_allocatedTime = INFINITE;
        m_forcedNodes = nodes;
    }
    inline void Infinite() {
        m_allocatedTime = INFINITE;
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
    bool NodeLimit() { return m_nodes >= m_forcedNodes; };

    //IterativeDeepening methods
    void ClearSearch();
    void UciOutput(std::string PV);

    //Debug
    void ShowDebugInfo();

    //Limits
    Limits m_limits;
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
    Move m_ponderMove;

    //Time management
    Utils::Clock m_clock;
    bool m_stop;
    int m_nodesTimeCheck;

    //Helpers
    int m_ply;
    int m_plyqs;
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

#endif //SEARCH_H
