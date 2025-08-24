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

    void Update(int ply, Move move, int score);

    Move PonderMove() const;
    std::string PVString() const;

private:
    using PVLine = std::array<Move, MAX_PLY>;
    using PVTable = std::array<PVLine, MAX_PLY>;

    PVTable m_pvTable;
    std::array<int, MAX_PLY> m_pvLength;
};
