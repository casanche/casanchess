#include "SearchLimits.h"

namespace {
    constexpr int TIME_OVERHEAD = 50; //ms
    constexpr int TIMEOVER_CHECK_NODES = 1024; // Check time every N nodes
}

void Limits::StartNewSearch(COLOR color, const UCI_Limits& limits) {
    ResetSignals();
    AllocateLimits(color, limits);
    RestartClock();
}

bool Limits::LimitsReached(u64 nodes) {
    // Sent by interface
    if(m_stop)
        return true;
    if(m_ponderhit)
        Apply_PonderHit();

    // Fixed nodes
    if(nodes >= m_forcedNodes) {
        Stop();
        return true;
    }

    // Time. Calculate every N nodes to avoid expensive time checks.
    if(nodes >= m_nextTimeCheck) {
        m_nextTimeCheck = nodes + TIMEOVER_CHECK_NODES;
        i64 elapsedTime = UpdateElapsedTime();
        if(elapsedTime >= m_allocatedTime || elapsedTime >= m_forcedTime) {
            Stop();
            return true;
        }
    }

    return false;
}

i64 Limits::UpdateElapsedTime() {
    m_elapsedTime = m_clock.Elapsed();
    return m_elapsedTime;
}

uint Limits::CalculateNPS(u64 nodes) const {
    return static_cast<uint>(1000 * nodes / (m_elapsedTime+1));
}

void Limits::ResetSignals() {
    m_stop = false;
    m_ponderhit = false;
}

// =============
// == Private ==
// =============

// Set search limits from the UCI interface.
// For normal timed games, estimate the time to spend in the search (allocatedTime).
// Fixed modes:
// - infinite: search until manually stopped
// - depth: search for a fixed depth
// - moveTime: search for a fixed time
// - nodes: search for a fixed number of nodes
void Limits::AllocateLimits(COLOR color, const UCI_Limits& limits) {
    m_limits = limits;
    m_color = color;

    // Default to infinite search
    Infinite();

    // Special search modes
    if(limits.infinite) { return; }
    if(limits.ponder)   { return; }
    if(limits.depth)    { m_forcedDepth = limits.depth; return; }
    if(limits.moveTime) { m_forcedTime = limits.moveTime; return; }
    if(limits.nodes)    { m_forcedNodes = limits.nodes; return; }

    // Time estimation in normal games
    constexpr int ESTIMATED_MOVESTOGO = 20;
    const int movesToGo = limits.movesToGo ? limits.movesToGo
                                           : ESTIMATED_MOVESTOGO;

    int myTime   = (color == WHITE) ? limits.wtime : limits.btime;
    // int yourTime = (color == WHITE) ? limits.btime : limits.wtime;
    int myInc    = (color == WHITE) ? limits.winc  : limits.binc;

    // Move overhead to account for communication delays
    const int myTimeSafe = std::max(0, myTime - TIME_OVERHEAD);

    m_allocatedTime = myTimeSafe / movesToGo + myInc;
}

void Limits::Infinite() {
    m_forcedDepth = MAX_DEPTH;
    m_forcedTime = INFINITE_I64;
    m_forcedNodes = INFINITE_U64;
    m_allocatedTime = INFINITE_I64;
}

void Limits::Apply_PonderHit() {
    assert(m_limits.ponder && m_limits.ponderhit);

    m_ponderhit = false;

    m_limits.ponder = false;
    m_limits.ponderhit = true;

    AllocateLimits(m_color, m_limits);

    RestartClock(); // without ResetSignals()
}

void Limits::RestartClock() {
    m_elapsedTime = 0;
    m_nextTimeCheck = TIMEOVER_CHECK_NODES;

    m_clock.Start();
}
