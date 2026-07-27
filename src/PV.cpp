#include "PV.h"

PV::PV() {
    ClearTable();
}

void PV::ClearTable() {
    for(int i = 0; i <= MAX_PLY; ++i) {
        ClearPly(i);
    }
}

// Clear the PV for the current ply
void PV::ClearPly(int ply) {
    assert(ply >= 0 && ply <= MAX_PLY);
    m_pvLength[ply] = 0;
}

void PV::Update(int ply, Move move, [[maybe_unused]] int score) {
    m_pvTable[ply][0] = move;

    const int childPly = ply + 1;
    for(int i = 0; i < m_pvLength[childPly]; ++i) {
        m_pvTable[ply][i + 1] = m_pvTable[childPly][i];
    }

    m_pvLength[ply] = m_pvLength[childPly] + 1;
}

Move PV::PonderMove() const {
    if(m_pvLength[0] > 1)
        return m_pvTable[0][1];
    return Move();
}

std::string PV::PVString() const {
    std::string pv;
    for(int i = 0; i < m_pvLength[0]; ++i) {
        pv += m_pvTable[0][i].Notation();
        pv += " ";
    }
    return pv;
}
