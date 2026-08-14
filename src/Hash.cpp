#include "Hash.h"

#include "Debug.h"

#include <bit>

// =========================
// == Transposition table ==
// =========================

void TTEntry::Clear() {
    zkey = 0;
    score = NO_SCORE;
    eval = NO_EVAL;
    padding = 0;
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

void TT::Store(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age, int eval) {
    assert(abs(score) <= MATESCORE_MAX);
    assert(depth <= MAX_DEPTH);

    u64 index = zkey & m_mask;
    TTEntry* entry = &m_entries[index];

    // Age bitfield protection
    const u8 ttAge = age & 0x3F;

    //Replacement scheme
    const bool older = ttAge != entry-> age;
    const bool higherDepth = depth >= entry->depth;

    const bool replace = older || higherDepth;
    if(replace) {
        const bool zkeyMatch = (UpperBits<u32>(zkey) == entry->zkey);
    
        entry->zkey = UpperBits<u32>(zkey);
        entry->score = SafeCastInt16( ScoreToHash(score, ply) );
        entry->eval = SafeCastInt16(eval);
        entry->depth = SafeCastU8(depth);
        entry->type = type;
        entry->age = ttAge;

        // Do not overwrite a valid bestMove with a null move for the same position
        if(bestMove.MoveType() != MOVE_TYPE::NULLMOVE || !zkeyMatch)
            entry->bestMove = bestMove;
    }
}

TTEntry* TT::Probe(u64 zkey) {
    u64 index = zkey & m_mask;
    TTEntry* entry = &m_entries[index];

    const bool zkeyMatch = (UpperBits<u32>(zkey) == entry->zkey);
    if(zkeyMatch)
        return entry;

    return nullptr;
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
    static_assert( std::has_single_bit(EVALCACHE_ENTRIES), "EVALCACHE_ENTRIES should be a power of 2." );

    m_size = EVALCACHE_ENTRIES;
    m_mask = m_size - 1;
    Clear();
}

void EvalCache::Store(u64 zkey, int eval) {
    assert(abs(eval) < MATESCORE_MAX);

    u64 index = zkey & m_mask;
    EvalEntry& entry = m_evalEntries[index];

    // Always-replace strategy
    entry.zkey = UpperBits<u32>(zkey);
    entry.eval = SafeCastInt16(eval);
}

bool EvalCache::Probe(u64 zkey, int& eval) {
    u64 index = zkey & m_mask;
    EvalEntry& entry = m_evalEntries[index];

    if(entry.zkey == UpperBits<u32>(zkey)) {
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
        count += (m_evalEntries[i].zkey != 0);
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
