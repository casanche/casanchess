#include "Hash.h"

// -- Transposition table

void TTEntry::Clear() {
    zkey = 0;
    score = 0;
    depth = 0;
    type = TTENTRY_TYPE::NONE;
    age = 0;
    bestMove = Move();
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

    //Replacement scheme
    if(age != m_entries[index].age || depth >= m_entries[index].depth) {
        TTEntry entry;
        entry.zkey = zkey;
        entry.score = score;
        entry.depth = depth;
        entry.type = type;
        entry.age = age;
        entry.bestMove = bestMove;

        m_entries[index] = entry;
    }
}

TTEntry* TT::ProbeEntry(U64 zkey, int depth) {
    U64 index = zkey % MAX_HASH_ENTRIES;
    TTEntry entry = m_entries[index];
    if(entry.zkey == zkey && entry.depth >= depth) {
        return &m_entries[index];
    } else {
        return nullptr;
    }
}

int TT::OccupancyPerMil() {
    int count = 0;
    for(int i = 0; i < 1000; i++) {
        count += (m_entries[i].zkey != 0);
    }
    return count;
}

U64 TT::NumEntries() {
    U64 count = 0;
    for(U64 i = 0; i < MAX_HASH_ENTRIES; ++i) {
        // count += (m_entries[i].type != TTENTRY_TYPE::NONE);
        count += (m_entries[i].zkey != 0);
    }
    return count;
}

// -- Pawn-hash

PawnHash Hash::pawnHash;

void PawnEntry::Clear() {
    zkey = 0;
    evalMg = 0;
    evalEg = 0;
}

PawnHash::PawnHash() {
    m_pawnEntries = new PawnEntry[PAWN_HASH_SIZE];
    Clear();
}

PawnHash::~PawnHash() {
    delete [] m_pawnEntries;
}

void PawnHash::Clear() {
    for(U64 i=0; i < PAWN_HASH_SIZE; ++i) {
        m_pawnEntries[i].Clear();
    }
}

void PawnHash::AddEntry(U64 zkey, int evalMg, int evalEg) {
    assert( abs(evalMg) <= INFINITE_SCORE );
    assert( abs(evalEg) <= INFINITE_SCORE );

    U64 index = zkey % PAWN_HASH_SIZE;

    PawnEntry pawnEntry;
    pawnEntry.zkey = zkey;
    pawnEntry.evalMg = evalMg;
    pawnEntry.evalEg = evalEg;

    m_pawnEntries[index] = pawnEntry;
}

PawnEntry* PawnHash::ProbeEntry(U64 zkey) {
    U64 index = zkey % PAWN_HASH_SIZE;
    PawnEntry entry = m_pawnEntries[index];
    if(entry.zkey == zkey) {
        return &m_pawnEntries[index];
    } else {
        return nullptr;
    }
}