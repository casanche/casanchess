#pragma once

#include "Board.h"
#include "Constants.h"
#include "Debug.h"
#include "Heuristics.h"
#include "Move.h"
#include "PV.h"
#include "Utils.h"

#include <vector>

const int MAX_ROOTMOVES = 256;

// Search limits from UCI
struct Limits {
    bool infinite = false;
    bool ponderhit = false;
    int depth = 0;
    int nodes = 0;
    int moveTime = 0;

    int wtime = 0;
    int btime = 0;

    int winc = 0;
    int binc = 0;

    int movesToGo = 0;
};

class Search {
public:
    Search();

    // Start search
    void IterativeDeepening(Board &board, bool fullSearchClearFlag = false);

    // Flow
    void ClearSearch(bool fullSearchClearFlag);
    int64_t ElapsedTime() { return m_clock.Elapsed(); }
    void Stop() { m_stop = true; }
    void DebugMode() { m_debugMode = true; }

    // Limits management
    Limits GetLimits() { return m_limits; }
    void AllocateLimits(Board &board, const Limits& limits);
    void FixDepth(int depth);
    void FixTime(int time); // (ms)
    void FixNodes(int nodes);
    void Infinite();

    // Getters
    Move BestMove() const { return m_bestMove; };
    int BestScore() const { return m_bestScore; };
    u64 GetNodes() const { return m_nodes; };
    int GetNps() const { return m_nps; };

    // Interface
    void MakeMove(Board &board) { board.MakeMove(m_bestMove); };

private:
    // Internal search algorithms
    int AspirationWindow(Board& board, const int depth, const int bestScore);
    int RootMax(Board &board, int depth, int alpha, int beta);
    int NegaMax(Board  &board, int depth, int alpha, int beta);
    int QuiescenceSearch(Board &board, int alpha, int beta);

    // IterativeDeepening methods
    void UciOutput(std::string PV);

    // NegaMax methods
    int LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV);

    // Limits
    bool TimeOver();
    bool NodeLimit() { return m_nodes >= m_forcedNodes; };

    // Debug
    void ShowDebugInfo();

    // --- Private variables ---

    // Search state
    PV m_pv;
    int m_bestScore; // Best score found so far for the current search
    Move m_bestMove; // Best move found so far for the current search
    bool m_nullmoveAllowed; // Prevents two consecutive null moves
    u8 m_searchCount; // Used as 'age' in transposition tables
    // Nodes
    u64 m_nodes; // Number of nodes searched
    int m_nps; // Nodes per second
    uint m_tbHits; // Number of endgame table hits
    // Depth
    int m_depth; // Current search depth, in plies, for this iteration
    int m_ply; // Distance from root
    int m_plyqs; // Distance within the Quiescence Search
    int m_selPly; // Maximum ply reached (UCI reporting)

    // Limits
    Limits m_limits;
    int m_maxDepth; // Fixed depth limit
    int m_forcedTime; // Fixed time limit (ms)
    u64 m_forcedNodes; // Fixed nodes limit
    bool m_stop; // Flag to indicate search stop
    
    // Time management
    Utils::Clock m_clock;
    int64_t m_elapsedTime; // Time passed since the start of the search (ms)
    int m_allocatedTime; // In normal timed games, estimation of the time to use within the search (ms)
    int m_nodesTimeCheck; // Number of nodes searched since the last time check

    // Heuristics
    Heuristics m_heuristics; // Heuristics for move ordering (history, killers)

    // Debug
    bool m_debugMode; // UCI debug mode
    SearchDebug m_debug;
};
