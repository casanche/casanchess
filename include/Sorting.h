#ifndef SORTING_H
#define SORTING_H

#include "Move.h"
struct RootMove;

class Board;
class TT;
struct Heuristics;

const int MAX_PLY = 128;
const int RESET_HISTORY = INFINITE / 30;

namespace Sorting {
    void SortCaptures(Board &board, MoveList &moveList);
    void SortEvasions(Board &board, MoveList &moveList);
    void SortMoves(Board &board, MoveList& moveList, TT& tt, const Heuristics &heuristics, int ply);
    void SortRootMoves(std::vector<RootMove>& rootMoves);
}

class KillerHeuristics {
public:
    void Clear() {
        for(int i = 0; i < MAX_PLY; i++) {
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
    Move m_killers[MAX_PLY][2]; //[PLY][SLOT]
};

//Loop over all elements of the history table, to perform operations
#define LoopHistoryTable(stmt) \
    for(COLORS color : {WHITE, BLACK}) { \
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
    }
    void Clear() {
        LoopHistoryTable(m_history[color][from][to] = 0);
        m_maxValue = 0;
    }
    void GoodHistory(const Move& move, COLORS color, int depth) {
        m_history[color][move.FromSq()][move.ToSq()] += depth*depth;
        TestOverflow(move, color);
    }
    void BadHistory(const Move& move, COLORS color, int depth) {
        m_history[color][move.FromSq()][move.ToSq()] -= depth*depth;
        TestOverflow(move, color);
    }
    int Get(const Move& move, COLORS color) const {
        return m_history[color][move.FromSq()][move.ToSq()];
    }
    int MaxValue() const {
        return m_maxValue;
    }
private:
    int m_history[2][64][64]; //[COLOR][SQUARE][SQUARE]
    int m_maxValue;
    void TestOverflow(const Move& move, COLORS activeColor) {
        int value = m_history[activeColor][move.FromSq()][move.ToSq()];
        if(value > m_maxValue) {
            m_maxValue = value;
        }
        if(m_maxValue > RESET_HISTORY) {
            LoopHistoryTable(m_history[color][from][to] /= 3);
        }
    }
};

struct Heuristics {
    KillerHeuristics killer;
    HistoryHeuristics history;
};

#endif //SORTING_H
