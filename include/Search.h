#pragma once

#include "Board.h"
#include "Constants.h"
#include "Debug.h"
#include "Hash.h"
#include "Heuristics.h"
#include "Move.h"
#include "PV.h"
#include "SearchLimits.h"
#include "Utils.h"

#include <vector>

const int MAX_ROOTMOVES = 256;

using BOUND_TYPE = TTENTRY_TYPE;

class Search {
public:
    Search(TT& tt);

    // Copy forbidden
    Search(const Search&) = delete;
    Search& operator=(const Search&) = delete;

    void ClearSearch(bool fullSearchClearFlag);
    void ClearEvalCache() { m_evalCache.Clear(); }

    // Start search
    void IterativeDeepening(Board &board, const UCI_Limits& limits, bool fullSearchClearFlag = false);

    // Flow
    void PonderHit() { m_limits.PonderHit(); }
    void Stop() { m_limits.Stop(); }

    // Limits management
    const Limits& GetLimits() const { return m_limits; }

    // Getters
    Move BestMove() const { return m_bestMove; };
    int BestScore() const { return m_bestScore; };
    u64 GetNodes() const { return m_nodes; };

    // Interface
    void MakeMove(Board &board) { board.MakeMove(m_bestMove); };
    void ShowHashMoves(Board& board); // Use the context of the last search

private:
    // Internal search algorithms
    int AspirationWindow(Board& board, const int depth, const int bestScore);
    int RootMax(Board &board, int depth, int alpha, int beta);
    int NegaMax(Board  &board, int depth, int alpha, int beta);
    int QuiescenceSearch(Board &board, int alpha, int beta);

    // Root-relative search policy and cache keys
    void SetRootContext(const Board& board);
    static int ApplyAmbition(EvaluationOutput output, int ambition, bool rootToMove);
    int Evaluate(const Board& board) const;
    int StaticEvaluation(const Board& board, int ttEval);
    int DrawScore(const Board& board) const;

    // Use contextual keys for caches; preserve board keys for repetition detection.
    u64 TTKey(const Board& board) const { return board.ZKey() ^ m_ttContext; }
    u64 EvalKey(const Board& board) const { return board.ZKey() ^ m_evalContext; }

    // NegaMax methods
    int LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV);

    // Debug
    void ShowDebugInfo();

    // --- Private variables ---

    // Search state
    PV m_pv;
    int m_bestScore; // Best score found so far for the current search
    Move m_bestMove; // Best move found so far for the current search
    bool m_nullmoveAllowed; // Prevents two consecutive null moves
    u8 m_searchCount; // Used as 'age' in transposition tables
    COLOR m_rootPlayer = WHITE;
    u64 m_ttContext = 0;
    u64 m_evalContext = 0;
    // Nodes
    u64 m_nodes; // Number of nodes searched
    uint m_tbHits; // Number of endgame table hits
    // Depth
    int m_depth; // Current search depth, in plies, for this iteration
    int m_ply; // Distance from root
    int m_plyqs; // Distance within the Quiescence Search
    int m_selPly; // Maximum ply reached (UCI reporting)

    // Limits
    Limits m_limits;

    // Hash tables
    TT& m_tt;
    EvalCache m_evalCache;

    // Heuristics
    Heuristics m_heuristics; // Heuristics for move ordering (history, killers)

    // Debug
    SearchDebug m_debug;

    friend class Datagen;
};
