#pragma once

#include "Move.h"
#include "MoveGenerator.h"

#include <bit>
#include <cassert>

class Board;
class TT;
struct Heuristics;

const int MAX_BONUS = 400;
constexpr int MAX_HISTORY_VALUE = std::bit_floor( (uint)INFINITE / MAX_BONUS );

namespace Sorting {
    void SortMoves(Board &board, MoveList &moves, Move hashMove, const Heuristics &heuristics, int ply);
    void SortEvasions(Board &board, MoveList &moves, Move hashMove);
    void SortTactical(Board &board, MoveList &moves, Move hashMove);
}

class KillerHeuristics {
public:
    void Clear() {
        for(auto& plyKillers : m_killers) {
            plyKillers[0] = Move();
            plyKillers[1] = Move();
        }
    }
    void Update(const Move& killer, int ply) {
        assert(ply >= 0 && ply < MAX_PLY);
        if(killer != m_killers[ply][0]) {
            m_killers[ply][1] = m_killers[ply][0];
            m_killers[ply][0] = killer;
        }
    }
    Move Primary(int ply) const {
        assert(ply >= 0 && ply < MAX_PLY);
        return m_killers[ply][0];
    }
    Move Secondary(int ply) const {
        assert(ply >= 0 && ply < MAX_PLY);
        return m_killers[ply][1];
    }
private:
    Move m_killers[MAX_PLY][2]; //[PLY][SLOT]
};

class HistoryHeuristics {
public:
    void Age() {
        ApplyToAll([](int& historyValue) { historyValue /= 8; });
        m_maxValue /= 8;
    }
    void Clear() {
        ApplyToAll([](int& historyValue) { historyValue = 0; });
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
    template<typename Func>
    void ApplyToAll(Func operation) {
        for(auto& colorTable : m_history) {
            for(auto& fromTable : colorTable) {
                for(int& historyValue : fromTable) {
                    operation(historyValue);
                }
            }
        }
    }

    int Bonus(int depth) const {
        int bonus = depth * depth;
        return std::min(MAX_BONUS, bonus);
    }
    void UpdateMaxValue(int historyValue) {
        if(historyValue > m_maxValue) {
            m_maxValue = historyValue;
        }
    }

    int m_history[2][64][64] = {}; //[COLOR][SQUARE_FROM][SQUARE_TO]
    int m_maxValue = 0;
};

struct Heuristics {
    KillerHeuristics killer;
    HistoryHeuristics history;
};
