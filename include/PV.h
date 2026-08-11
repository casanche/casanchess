#pragma once

#include "Constants.h"
#include "Move.h"

#include <array>
#include <string>

class PV {
public:
    PV();
    void ClearTable();
    void ClearPly(int ply);

    void Update(int ply, Move move);

    std::string PonderString() const;
    std::string PVString() const;

private:
    using PVLine = std::array<Move, MAX_PLY + 1>;
    using PVTable = std::array<PVLine, MAX_PLY + 1>; // +1 to allow safe read of childPly

    PVTable m_pvTable;
    std::array<int, MAX_PLY + 1> m_pvLength;
};
