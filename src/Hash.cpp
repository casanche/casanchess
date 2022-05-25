#include "Hash.h"

// Extern declarations
TT Hash::tt;
PawnHash Hash::pawnHash;

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
    m_entries = nullptr;
    SetSize(DEFAULT_HASH_SIZE);
}

TT::~TT() {
    delete [] m_entries;
}

void TT::Clear() {
    for(u64 i=0; i < m_size; ++i) {
        m_entries[i].Clear();
    }
}

//Store mates in hash as relative from the search position (POS)
//ROOT ---- (Mate in X+ply) ---- POS ---- (Mate in X) ---- MATE
int TT::ScoreToHash(int score, int ply) {
    if(IsMateValue(score)) {
        if(score > 0)   return score + ply;
        else            return score - ply;
    }
    return score;
}

//Translate mate scores as relative from the root (ROOT')
//ROOT' ---- (Mate in X+ply') ---- POS ---- (Mate in X) ---- MATE
int TT::ScoreFromHash(int score, int ply) {
    if(IsMateValue(score)) {
        if(score > 0)   return score - ply;
        else            return score + ply;
    }
    return score;
}

void TT::AddEntry(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age) {
    assert(abs(score) <= MATESCORE);
    assert(depth <= MAX_DEPTH);

    u64 index = zkey % m_size;

    //Replacement scheme
    if(age != m_entries[index].age || depth >= m_entries[index].depth) {
        TTEntry entry;
        entry.zkey = zkey;
        entry.score = ScoreToHash(score, ply);
        entry.depth = depth;
        entry.type = type;
        entry.age = age;
        entry.bestMove = bestMove;

        m_entries[index] = entry;
    }
}

TTEntry* TT::ProbeEntry(u64 zkey, int depth) {
    u64 index = zkey % m_size;
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

u64 TT::NumEntries() {
    u64 count = 0;
    for(u64 i = 0; i < m_size; ++i) {
        count += (m_entries[i].zkey != 0);
    }
    return count;
}

void TT::SetSize(int size) {  // size in MB
    const u64 hashEntries = size * (1024*1024) / sizeof(TTEntry);

    delete [] m_entries;
    m_entries = new TTEntry[hashEntries];
    
    m_size = hashEntries;
    Clear();
}

// -- Pawn-hash

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
    for(u64 i=0; i < PAWN_HASH_SIZE; ++i) {
        m_pawnEntries[i].Clear();
    }
}

void PawnHash::AddEntry(u64 zkey, int evalMg, int evalEg) {
    assert( abs(evalMg) <= MATESCORE );
    assert( abs(evalEg) <= MATESCORE );

    u64 index = zkey % PAWN_HASH_SIZE;

    PawnEntry pawnEntry;
    pawnEntry.zkey = zkey;
    pawnEntry.evalMg = evalMg;
    pawnEntry.evalEg = evalEg;

    m_pawnEntries[index] = pawnEntry;
}

float PawnHash::Occupancy() {
    int count = 0;
    for(int i = 0; i < PAWN_HASH_SIZE; ++i) {
        count += (m_pawnEntries[i].zkey != 0);
    }
    return (float)count / PAWN_HASH_SIZE;
}

PawnEntry* PawnHash::ProbeEntry(u64 zkey) {
    u64 index = zkey % PAWN_HASH_SIZE;
    PawnEntry entry = m_pawnEntries[index];
    if(entry.zkey == zkey) {
        return &m_pawnEntries[index];
    } else {
        return nullptr;
    }
}
