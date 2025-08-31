#pragma once

#include "Move.h"
#include "MoveGenerator.h"

#include <bit>

class Board;
class TT;
struct Heuristics;

const int HEURISTICS_MAX_PLY = 128;
const int MAX_BONUS = 400;
constexpr int MAX_HISTORY_VALUE = std::bit_floor( (uint)INFINITE / MAX_BONUS );

namespace Sorting {
    void SortMoves(Board &board, MoveList &moves, TT& tt, const Heuristics &heuristics, int ply);
    void SortEvasions(Board &board, MoveList &moves);
    void SortTactical(Board &board, MoveList &moves);
}

class KillerHeuristics {
public:
    void Clear() {
        for(int i = 0; i < HEURISTICS_MAX_PLY; i++) {
            m_killers[i][0] = Move();
            m_killers[i][1] = Move();
        }
    }
    void Update(const Move& killer, int ply) {
        if(killer != m_killers[ply][0]) {
            m_killers[ply][1] = m_killers[ply][0];
            m_killers[ply][0] = killer;
        }
    }
    Move Primary(int ply) const {
        assert(ply>=0);
        return m_killers[ply][0];
    }
    Move Secondary(int ply) const {
        assert(ply>=0);
        return m_killers[ply][1];
    }
private:
    Move m_killers[HEURISTICS_MAX_PLY][2]; //[PLY][SLOT]
};

//Loop over all elements of the history table, to perform operations
#define LoopHistoryTable(stmt) \
    for(COLOR color : {WHITE, BLACK}) { \
        for(int from = 0; from < 64; from++) { \
            for (int to = 0; to < 64; to++) { \
                stmt; \
            } \
        } \
    }

class HistoryHeuristics {
public:
    void Age() {
        LoopHistoryTable(m_history[color][from][to] /= 8);
        m_maxValue /= 8;
    }
    void Clear() {
        LoopHistoryTable(m_history[color][from][to] = 0);
        m_maxValue = 0;
    }
    void GoodHistory(const Move& move, COLOR color, int depth) {
        int& historyValue = m_history[color][move.FromSq()][move.ToSq()];
        int bonus = Bonus(depth);
        historyValue += bonus - (bonus * historyValue) / MAX_HISTORY_VALUE; // Dumped update
        UpdateMaxValue(historyValue);
    }
    int Get(const Move& move, COLOR color) const {
        return m_history[color][move.FromSq()][move.ToSq()];
    }
    int MaxValue() const {
        return m_maxValue;
    }
private:
    int Bonus(int depth) const {
        int bonus = depth * depth;
        return std::min(MAX_BONUS, bonus);
    }
    void UpdateMaxValue(int historyValue) {
        if(historyValue > m_maxValue) {
            m_maxValue = historyValue;
        }
    }

    int m_history[2][64][64]; //[COLOR][SQUARE_FROM][SQUARE_TO]
    int m_maxValue;
};

struct Heuristics {
    KillerHeuristics killer;
    HistoryHeuristics history;
};
