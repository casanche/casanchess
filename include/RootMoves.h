#pragma once

#include "Board.h"
#include "Constants.h"
#include "Hash.h"
#include "Heuristics.h"
#include "Move.h"

#include <array>

struct RootMove {
    int score;
    int previousScore;
    Move move;
};

class RootMoves {
public:
    RootMoves() { Clear(); }

    void Clear() { m_count = 0; }

    void Generate(Board& board, TT& tt, const Heuristics& heuristics);
    void PromoteToTop(int moveNumber);
    void Sort();
    void StorePrevious();
    void Update(int moveNumber, int score);

    int Size() const { return m_count; }
    void Print() const;

    // For range-based for loops
    RootMove* begin() { return m_moves.data(); }
    RootMove* end()   { return m_moves.data() + m_count; }

    const RootMove* begin() const { return m_moves.data(); }
    const RootMove* end()   const { return m_moves.data() + m_count; }

private:
    std::array<RootMove, MAX_MOVES> m_moves;
    int m_count;
};
