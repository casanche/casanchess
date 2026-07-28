#include "RootMoves.h"

#include <algorithm>

void RootMoves::Generate(Board& board, TT& tt, const Heuristics& heuristics) {
    Clear();

    MoveList moves = MoveGenerator::GenerateMoves(board);
    Sorting::SortMoves(board, moves, tt, heuristics, 0);

    for(auto move : moves) {
        m_moves[m_count].move = move;
        m_moves[m_count].score = -INFINITE_SCORE;
        m_moves[m_count].previousScore = -INFINITE_SCORE;
        m_count++;
    }
}

// Moves the 'moveNumber' to the front of the array
// Note 'moveNumber' (1-based) is different from 'index' (0-based)
void RootMoves::PromoteToTop(int moveNumber) {
    int index = moveNumber - 1;
    assert(index >= 0 && index < m_count);

    if(index == 0) return; // Already at the top

    std::rotate(
        m_moves.begin(),
        m_moves.begin() + index,
        m_moves.begin() + index + 1
    );
}

void RootMoves::Sort() {
    if(m_count <= 1) return;

    std::stable_sort(m_moves.begin(), m_moves.begin() + m_count,
        [](const RootMove& a, const RootMove& b) {
            return a.score > b.score;
        }
    );
}

void RootMoves::StorePrevious() {
    for(int i = 0; i < m_count; ++i) {
        m_moves[i].previousScore = m_moves[i].score;
    }
}

void RootMoves::Update(int moveNumber, int score) {
    int index = moveNumber - 1;
    assert(index >= 0 && index < m_count);

    m_moves[index].score = score;
}

void RootMoves::Print() const {
    std::cout << "RootMoves (count: " << m_count << "):\n";
    int moveNumber = 0;
    for(const auto& rootmove : *this) {
        moveNumber++;
        std::cout << "Number: " << moveNumber << ", "
                    << "Move: " << rootmove.move.Notation() << ", "
                    << "Score: " << rootmove.score << ", "
                    << "Previous Score: " << rootmove.previousScore << "\n";
    }
}
