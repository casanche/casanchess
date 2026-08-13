#pragma once

#include "Constants.h"
#include "Utils.h"

#include <atomic>

// Search limits from UCI (input)
struct UCI_Limits {
    bool infinite = false;
    bool ponder = false;

    int depth = 0;
    int nodes = 0;
    int moveTime = 0;

    int wtime = 0;
    int btime = 0;

    int winc = 0;
    int binc = 0;

    int movesToGo = 0;

    static UCI_Limits FixDepth(int depth) { return UCI_Limits{ .depth = depth }; };
    static UCI_Limits FixTime(int time) { return UCI_Limits{ .moveTime = time }; };
    static UCI_Limits FixNodes(int nodes) { return UCI_Limits{ .nodes = nodes }; };
    static UCI_Limits Infinite() { return UCI_Limits{ .infinite = true }; };
};

// Management of search limits (transformed)
class Limits {
public:
    Limits() = default;

    void StartNewSearch(COLOR color, const UCI_Limits& limits, size_t movesSize);

    bool LimitsReached(u64 nodes);

    // Time management
    i64 UpdatedElapsedTime();
    uint CalculateNPS(u64 nodes) const;

    // UCI signals
    void ResetSignals();
    void PonderHit() { m_ponderhit = true; }
    void Stop() { m_stop = true; }

    // Getters
    i64 AllocatedTime() const { return m_allocatedTime; }
    i64 ElapsedTime() const { return m_elapsedTime; }
    int MaxDepth() const { return m_forcedDepth; }
    bool Stopped() const { return m_stop; }
    
private:
    // =============
    // == Methods ==
    // =============
    void AllocateLimits(COLOR color, const UCI_Limits& limits, size_t movesSize);

    void Infinite();
    
    void Apply_PonderHit();
    void RestartClock();

    // ===============
    // == Variables ==
    // ===============
    UCI_Limits m_limits;

    COLOR m_color;

    // Time management
    Utils::Clock m_clock;
    i64 m_elapsedTime = 0; // Time passed since the start of the search (ms)
    i64 m_allocatedTime = 0; // In normal timed games, estimation of the time to use within the search (ms)
    u64 m_nextTimeCheck = 0; // Next node count to check time (to avoid expensive time checks every node)

    // Fixed limits
    int m_forcedDepth = 0; // Fixed depth limit
    i64 m_forcedTime = 0; // Fixed time limit (ms)
    u64 m_forcedNodes = 0; // Fixed nodes limit

    // Root-level information
    size_t m_movesSize = 0; // Number of legal moves in the root position (used for time estimation)

    // UCI signals
    std::atomic<bool> m_stop = false; // Flag to stop the search (time limit, user input, etc.)
    std::atomic<bool> m_ponderhit = false; // Flag to indicate that the ponder move was played
};
