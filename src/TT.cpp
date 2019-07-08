#include "TT.h"

void TTEntry::Clear() {
    zkey = 0;
    score = 0;
    type = TTENTRY_TYPE::NONE;
    bestMove = Move();
    depth = 0;
    // age = 0;
}

TT::TT() {
    m_entries = new TTEntry[MAX_HASH_ENTRIES];
    Clear();
}

TT::~TT() {
    delete [] m_entries;
}

void TT::Clear() {
    for(U64 i=0; i < MAX_HASH_ENTRIES; ++i) {
        m_entries[i].Clear();
    }
}

void TT::AddEntry(U64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int age) {

    assert(bestMove.MoveType());
    assert(abs(score) <= INFINITE_SCORE);
    assert(depth <= MAX_DEPTH);

    U64 index = zkey % MAX_HASH_ENTRIES;
    // if(age < m_entries[index].age) return;

    TTEntry entry;
    entry.zkey = zkey;
    entry.score = score;
    entry.type = type;
    entry.bestMove = bestMove;
    entry.depth = depth;
    // entry.age = age;

    m_entries[index] = entry;
}

TTEntry* TT::ProbeEntry(U64 zkey, int depth) {
    U64 index = zkey % MAX_HASH_ENTRIES;
    TTEntry entry = m_entries[index];
    if(entry.zkey == zkey && entry.depth >= depth) { //&& entry.depth == depth
        return &m_entries[index];
    } else {
        return nullptr;
    }
}

U64 TT::NumEntries() {
    U64 count = 0;
    for(U64 i = 0; i < MAX_HASH_ENTRIES; ++i) {
        // count += (m_entries[i].type != TTENTRY_TYPE::NONE);
        count += (m_entries[i].zkey != 0);
    }
    return count;
}
