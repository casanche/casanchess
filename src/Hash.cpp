#include "Hash.h"

#include <bit>
#include <cstring>

// =========================
// == Transposition table ==
// =========================

void TTEntry::Clear() {
    zkey = 0;
    score = NO_SCORE;
    eval = NO_EVAL;
    wasPV = false;
    padding = 0;
    depth = 0;
    type = TTENTRY_TYPE::NONE;
    age = 0;
    bestMove = Move();
}

TT::TT() {
    m_buckets = nullptr;
    SetSize(DEFAULT_HASH_SIZE);
}

TT::~TT() {
    delete [] m_buckets;
}

void TT::Store(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, bool wasPV, int eval) {
    assert(abs(score) <= MATESCORE_MAX);
    assert(depth <= MAX_DEPTH);

    u64 bucketIndex = zkey & m_mask;
    TTEntry& entry = SelectEntry(m_buckets[bucketIndex], UpperBits<u32>(zkey));

    //Replacement scheme
    const bool older = entry.age != m_age;
    const bool higherDepth = depth >= entry.depth;

    const bool replace = older || higherDepth;
    if(replace) {
        const bool zkeyMatch = (UpperBits<u32>(zkey) == entry.zkey);
    
        entry.zkey = UpperBits<u32>(zkey);
        entry.score = SafeCastInt16( ScoreToHash(score, ply) );
        entry.eval = SafeCastInt16(eval);
        entry.depth = SafeCastU8(depth);
        entry.wasPV = wasPV;
        entry.type = type;
        entry.age = m_age;

        // Do not overwrite a valid bestMove with a null move for the same position
        if(bestMove.MoveType() != MOVE_TYPE::NULLMOVE || !zkeyMatch)
            entry.bestMove = bestMove;
    }
}

bool TT::Probe(u64 zkey, TTEntry& entry) const {
    u64 bucketIndex = zkey & m_mask;
    const TTBucket& bucket = m_buckets[bucketIndex];

    for(const TTEntry& stored : bucket.entries) {
        if(stored.zkey == UpperBits<u32>(zkey)) {
            entry = stored;
            return true;
        }
    }

    return false;
}

void TT::Clear() {
    for(u64 i = 0; i < m_size; ++i) {
        for(TTEntry& entry : m_buckets[i].entries) {
            entry.Clear();
        }
    }
}

void TT::NewSearch() {
    m_age = (m_age + 1) % TT_AGE_CYCLE;
}

// For a faster entry lookup using a mask: downsize buckets (m_size) to fill in a power of 2
void TT::SetSize(int sizeInMB) {
    assert(sizeInMB >= MIN_HASH_SIZE && sizeInMB <= MAX_HASH_SIZE);

    u64 maxBuckets = u64(sizeInMB) * (1024 * 1024) / sizeof(TTBucket);

    m_size = std::bit_floor(maxBuckets);
    m_mask = m_size - 1;

    delete [] m_buckets;
    m_buckets = new TTBucket[m_size];

    Clear();
}

int TT::Occupancy() const {
    constexpr int ENTRIES_SAMPLE = 1000;
    constexpr int BUCKETS_SAMPLE = ENTRIES_SAMPLE / 4;

    int count = 0;
    for(size_t i = 0; i < BUCKETS_SAMPLE; ++i) {
        for(const TTEntry& entry : m_buckets[i].entries) {
            count += (entry.zkey != 0);
        }
    }
    return count;
}

//Translate mate scores as relative from the root (ROOT')
//ROOT' <---- (Mate in X+ply') <---- POS <---- (Mate in X)
int TT::ScoreFromHash(int score, int ply) {
    assert(abs(score) <= MATESCORE_MAX);
    if(IsWinScore(score)) {
        if(score > 0) return score - ply;
        else          return score + ply;
    }
    return score;
}

//Store mates in hash as relative from the search position (POS)
//ROOT ----> (Mate in X+ply) ----> POS ----> (Mate in X)
int TT::ScoreToHash(int score, int ply) {
    assert(abs(score) <= MATESCORE_MAX);
    if(IsWinScore(score)) {
        if(score > 0) return score + ply;
        else          return score - ply;
    }
    return score;
}

// Exact match > Older > Shallower
TTEntry& TT::SelectEntry(TTBucket& bucket, u32 zkey) const {
    size_t selectedIndex = 0;

    for(size_t i = 0; i < bucket.entries.size(); ++i) {
        TTEntry& candidate = bucket.entries[i];
        const TTEntry& selected = bucket.entries[selectedIndex];

        if(candidate.zkey == zkey)
            return candidate; // Exact match
        
        const bool candidateOld = candidate.age != m_age;
        const bool selectedOld = selected.age != m_age;

        // Prefer older
        if(candidateOld && !selectedOld) {
            selectedIndex = i;
            continue;
        }
        if(!candidateOld && selectedOld)
            continue;

        // Prefer shallower
        if(candidate.depth < selected.depth)
            selectedIndex = i;
    }

    return bucket.entries[selectedIndex];
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
    assert(abs(eval) < WINSCORE);

    u64 index = zkey & m_mask;
    EvalEntry& entry = m_evalEntries[index];

    // Always-replace strategy
    entry.zkey = UpperBits<u32>(zkey);
    entry.eval = SafeCastInt16(eval);
}

bool EvalCache::Probe(u64 zkey, int& eval) const {
    u64 index = zkey & m_mask;
    const EvalEntry& entry = m_evalEntries[index];

    if(entry.zkey == UpperBits<u32>(zkey)) {
        eval = entry.eval;
        return true;
    }
    return false;
}

void EvalCache::Clear() {
    std::memset(m_evalEntries.get(), 0, sizeof(EvalEntry) * EVALCACHE_ENTRIES);
}

u64 EvalCache::Occupancy(u64 sampleSize) const {
    u64 count = 0;
    for(u64 i = 0; i < sampleSize; ++i) {
        count += (m_evalEntries[i].zkey != 0);
    }
    return count;
}
