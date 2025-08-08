#pragma once

#include "Constants.h"
#include "Move.h"

#include <array>
#include <span>

class Board;
struct MoveGenContext;

const int MAX_MOVES_RESERVE = 256;

class MoveList {
private:
    std::array<Move, MAX_MOVES_RESERVE> m_moves;
    size_t m_count = 0;

public:
    using iterator = Move*;
    using const_iterator = const Move*;

    iterator begin() { return m_moves.data(); }
    iterator end()   { return m_moves.data() + m_count; }

    const_iterator begin() const { return m_moves.data(); }
    const_iterator end()   const { return m_moves.data() + m_count; }

    void add(const Move& move) {
        assert(m_count < MAX_MOVES_RESERVE);
        m_moves[m_count++] = move;
    }
    void clear() { m_count = 0; }
    bool empty()  const { return m_count == 0; }
    size_t size() const { return m_count; }

    const Move& operator[](size_t index) const {
        assert(index < m_count);
        return m_moves[index];
    }
};

namespace MoveGenerator {
    // Generate all legal moves
    MoveList GenerateMoves(Board &board);
    
    // Generate check evasions when in check
    MoveList GenerateEvasionMoves(Board &board);
    // Generate tactical moves (captures and promotions)
    MoveList GenerateTacticalMoves(Board &board);
    
    // Pick a random move from the move list
    Move RandomMove(const MoveList& moveList);
}
