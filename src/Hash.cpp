#include "Hash.h"

#include "Debug.h"

#include <bit>

// =========================
// == Transposition table ==
// =========================

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

void TT::Store(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age) {
    assert(abs(score) <= MATESCORE_MAX);
    assert(depth <= MAX_DEPTH);

    u64 index = zkey & m_mask;
    TTEntry* entry = &m_entries[index];

    //Replacement scheme
    const bool older = age != entry-> age;
    const bool higherDepth = depth >= entry->depth;
    if(older || higherDepth) {
        entry->zkey = UpperBits(zkey);
        entry->score = SafeCastInt16( ScoreToHash(score, ply) );
        entry->depth = SafeCastU8(depth);
        entry->type = type;
        entry->age = age;
        entry->bestMove = bestMove;
    }
}

TTEntry* TT::Probe(u64 zkey, int depth) {
    u64 index = zkey & m_mask;
    TTEntry* entry = &m_entries[index];

    const bool zkeyEqual = UpperBits(zkey) == entry->zkey;
    const bool higherDepth = entry->depth >= depth;
    if(zkeyEqual && higherDepth) {
        return entry;
    } else {
        return nullptr;
    }
}

void TT::Clear() {
    for(u64 i=0; i < m_size; ++i) {
        m_entries[i].Clear();
    }
}

// For a faster entry lookup using a mask: downsize entries (m_size) to fill in a power of 2
void TT::SetSize(int sizeInMB) {
    u64 maxEntries = sizeInMB * (1024 * 1024) / sizeof(TTEntry);

    m_size = std::bit_floor(maxEntries);
    m_mask = m_size - 1;

    delete [] m_entries;
    m_entries = new TTEntry[m_size];

    Clear();
}

u64 TT::Occupancy(u64 sampleSize) const {
    u64 count = 0;
    for(u64 i = 0; i < sampleSize; ++i) {
        count += (m_entries[i].zkey != 0);
    }
    return count;
}

u32 TT::UpperBits(u64 zkey) {
    return static_cast<u32>(zkey >> 32);
}

//Translate mate scores as relative from the root (ROOT')
//ROOT' <---- (Mate in X+ply') <---- POS <---- (Mate in X)
int TT::ScoreFromHash(int score, int ply) {
    assert(abs(score) <= MATESCORE_MAX);
    if(IsWinValue(score)) {
        if(score > 0) return score - ply;
        else          return score + ply;
    }
    return score;
}

//Store mates in hash as relative from the search position (POS)
//ROOT ----> (Mate in X+ply) ----> POS ----> (Mate in X)
int TT::ScoreToHash(int score, int ply) {
    assert(abs(score) <= MATESCORE_MAX);
    if(IsWinValue(score)) {
        if(score > 0) return score + ply;
        else          return score - ply;
    }
    return score;
}

// ================
// == Eval cache ==
// ================

EvalCache::EvalCache() {
    m_size = EVALCACHE_ENTRIES;
    m_mask = m_size - 1;
    Clear();
}

void EvalCache::Store(u64 zkey, int eval) {
    assert(abs(eval) < MATESCORE_MAX);

    EvalEntry& entry = m_evalEntries[zkey & m_mask];

    // Always-replace strategy
    entry.zkey32 = static_cast<u32>(zkey);
    entry.eval = SafeCastInt16(eval);
}

bool EvalCache::Probe(u64 zkey, int& eval) {
    u32 key32 = static_cast<u32>(zkey);
    EvalEntry& entry = m_evalEntries[zkey & m_mask];

    if(entry.zkey32 == key32) {
        eval = entry.eval;
        return true;
    }
    return false;
}

void EvalCache::Clear() {
    for(auto& entry : m_evalEntries) {
        entry = {};
    }
}

u64 EvalCache::Occupancy(u64 sampleSize) const {
    u64 count = 0;
    for(u64 i = 0; i < sampleSize; ++i) {
        count += (m_evalEntries[i].zkey32 != 0);
    }
    return count;
}

// ===============
// == Pawn-hash ==
// ===============

namespace Hash {

    PawnHash::PawnHash() {
        m_pawnEntries = new PawnEntry[PAWN_HASH_SIZE];
        Clear();
    }
    
    PawnHash::~PawnHash() {
        delete [] m_pawnEntries;
    }
    
    void PawnHash::Clear() {
        for(u64 i=0; i < PAWN_HASH_SIZE; ++i) {
            m_pawnEntries[i] = {};
        }
    }
    
    void PawnHash::Store(u64 zkey, int evalMg, int evalEg) {
        u64 index = zkey % PAWN_HASH_SIZE;
    
        PawnEntry pawnEntry;
        pawnEntry.zkey = zkey;
        pawnEntry.evalMg = SafeCastInt16(evalMg);
        pawnEntry.evalEg = SafeCastInt16(evalEg);
    
        m_pawnEntries[index] = pawnEntry;
    }
    
    PawnEntry* PawnHash::Probe(u64 zkey) {
        u64 index = zkey % PAWN_HASH_SIZE;
        PawnEntry entry = m_pawnEntries[index];
        if(entry.zkey == zkey) {
            return &m_pawnEntries[index];
        } else {
            return nullptr;
        }
    }
    
    u64 PawnHash::Occupancy() const {
        u64 count = 0;
        for(u64 i = 0; i < PAWN_HASH_SIZE; ++i) {
            count += (m_pawnEntries[i].zkey != 0);
        }
        return count;
    }

}
