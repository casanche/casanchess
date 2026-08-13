#include "SearchLimits.h"

namespace {
    constexpr int TIME_OVERHEAD = 50; //ms
    constexpr int STOP_CHECK_NODES = 1024; // Check 'stop' conditions every N nodes
}

void Limits::StartNewSearch(COLOR color, const UCI_Limits& limits, size_t movesSize) {
    m_movesSize = movesSize;

    ResetSignals();
    AllocateLimits(color, limits, m_movesSize);
    RestartClock();
}

bool Limits::LimitsReached(u64 nodes) {
    // Fixed nodes
    if(nodes >= m_forcedNodes) {
        Stop();
        return true;
    }

    // Calculate expensive 'stop' conditions every N nodes
    if(nodes >= m_nextStopCheck) {
        // Signal checks sent by interface
        if(m_stop)
            return true;
        if(m_ponderhit)
            Apply_PonderHit();

        // Time checks
        i64 elapsedTime = UpdatedElapsedTime();
        if(elapsedTime >= m_allocatedTime || elapsedTime >= m_forcedTime) {
            Stop();
            return true;
        }

        m_nextStopCheck = nodes + STOP_CHECK_NODES;
    }

    return false;
}

i64 Limits::UpdatedElapsedTime() {
    m_elapsedTime = m_clock.Elapsed();
    return m_elapsedTime;
}

uint Limits::CalculateNPS(u64 nodes) const {
    return static_cast<uint>(1000 * nodes / (m_elapsedTime+1));
}

void Limits::PonderHit() {
    m_ponderhit = true;

    m_wakeup = true;
    m_wakeup.notify_all();
}

void Limits::ResetSignals() {
    m_stop = false;
    m_ponderhit = false;

    m_wakeup = false;
}

void Limits::Stop() {
    m_stop = true;

    m_wakeup = true;
    m_wakeup.notify_all();
}

void Limits::WaitIfNecessary() {
    while(!m_stop && (m_uciLimits.infinite || m_uciLimits.ponder)) {
        m_wakeup.wait(false);
    }
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
void Limits::AllocateLimits(COLOR color, const UCI_Limits& uciLimits, size_t movesSize) {
    m_uciLimits = uciLimits;
    m_color = color;

    // Default to infinite search
    Infinite();

    // Special search modes
    if(uciLimits.infinite) { return; }
    if(uciLimits.ponder)   { return; }
    if(uciLimits.depth)    { m_forcedDepth = uciLimits.depth; return; }
    if(uciLimits.moveTime) { m_forcedTime = uciLimits.moveTime; return; }
    if(uciLimits.nodes)    { m_forcedNodes = uciLimits.nodes; return; }
    
    // Time estimation in normal games
    int myTime   = (color == WHITE) ? uciLimits.wtime : uciLimits.btime;
    // int yourTime = (color == WHITE) ? uciLimits.btime : uciLimits.wtime;
    int myInc    = (color == WHITE) ? uciLimits.winc  : uciLimits.binc;

    // Move overhead to account for communication delays
    const int myTimeSafe = std::max(0, myTime - TIME_OVERHEAD);

    constexpr int ESTIMATED_MOVESTOGO = 20;
    const int movesToGo = uciLimits.movesToGo ? uciLimits.movesToGo
                                               : ESTIMATED_MOVESTOGO;

    m_allocatedTime = (myTimeSafe / movesToGo) + myInc;

    // Safety net: don't use more than the remaining time
    m_allocatedTime = std::min(m_allocatedTime, (i64)myTimeSafe);

    // Extra adjustments: root-level information
    if(movesSize == 1)
        m_allocatedTime /= 3;
}

void Limits::Infinite() {
    m_forcedDepth = MAX_DEPTH;
    m_forcedTime = INFINITE_I64;
    m_forcedNodes = INFINITE_U64;
    m_allocatedTime = INFINITE_I64;
}

void Limits::Apply_PonderHit() {
    assert(m_ponderhit && m_uciLimits.ponder);

    m_ponderhit = false;
    m_uciLimits.ponder = false;

    AllocateLimits(m_color, m_uciLimits, m_movesSize);

    if(m_allocatedTime == INFINITE_I64) return;

    // Extend allocated time
    m_allocatedTime += UpdatedElapsedTime() / 2;
}

void Limits::RestartClock() {
    m_elapsedTime = 0;
    m_nextStopCheck = STOP_CHECK_NODES;

    m_clock.Start();
}
