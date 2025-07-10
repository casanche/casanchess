#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "Hash.h"
#include "Heuristics.h"
#include "Move.h"
#include "Utils.h"

#include <vector>

const int MAX_ROOTMOVES = 256;

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

class SearchDebug {
public:
    void Increment(std::string theVariable) { debugVariables[theVariable]++; };
    void Print();
private:
    void Transform();
    
    std::map<std::string, int> debugVariables;
};

class Search {
public:
    Search();

    //Start search
    void IterativeDeepening(Board &board);

    //Flow
    int64_t ElapsedTime() { return m_clock.Elapsed(); }
    void Stop() { m_stop = true; }
    void DebugMode() { m_debugMode = true; }

    //Limits
    Limits GetLimits() { return m_limits; }
    void AllocateLimits(Board &board, Limits limits);
    void FixDepth(int depth);
    void FixTime(int time); //In milliseconds
    void FixNodes(int nodes);
    void Infinite();

    //Getters
    Move BestMove() const { return m_bestMove; };
    int BestScore() const { return m_bestScore; };
    u64 GetNodes() const { return m_nodes; };
    int GetNps() const { return m_nps; };

    //Interface
    void MakeMove(Board &board) { board.MakeMove(m_bestMove); };

private:
    int RootMax(Board &board, int depth, int alpha, int beta);
    int NegaMax(Board  &board, int depth, int alpha, int beta);
    int QuiescenceSearch(Board &board, int alpha, int beta);

    int LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV);

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
    u8 m_searchCount;
    bool m_nullmoveAllowed;

    //Heuristics
    Heuristics m_heuristics;

    //Debug
    bool m_debugMode;
    SearchDebug m_debug;
};

#endif //SEARCH_H
