#pragma once

#include "Constants.h"
#include "Move.h"

constexpr uint DEFAULT_HASH_SIZE = 16; //In MegaBytes
constexpr int PAWN_HASH_SIZE = 8192; //In number of entries

// 2^19 entries = 2 MB / 32 bits per entry
constexpr u64 EVALCACHE_ENTRIES = 1 << 19;

// =========================
// == Transposition table ==
// =========================

// Alpha node: the true eval is at most equal to the score (true <= score) UPPER_BOUND
// Beta node: the true eval is at least equal to the score (true >= score) LOWER_BOUND
enum class TTENTRY_TYPE : u8 { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

// 32(zkey) + 32(move) + 16(score) + 8(depth) + 2(type) + 6(age) + 32(padding) = 128 bits per entry
struct TTEntry {
    u32 zkey;
    u32 padding;
    i16 score;
    u8 depth;
    TTENTRY_TYPE type : 2;
    u8 age : 6;
    Move bestMove;

    void Clear();
};

class TT {
public:
    TT();
    ~TT();

    void Store(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age);
    TTEntry* Probe(u64 zkey, int depth);

    void Clear();
    void SetSize(int sizeInMB);

    u64 Size() { return m_size; };
    u64 Occupancy(u64 sampleSize = 1000) const;
    int ScoreFromHash(int score, int ply);

private:
    int ScoreToHash(int score, int ply);

    TTEntry* m_entries;
    u64 m_size; // Number of entries
    u64 m_mask; // Mask for AND operations
};

// ================
// == Eval cache ==
// ================

// 16(zkey) + 16(eval) = 32 bits per entry
struct EvalEntry {
    u16 zkey;
    i16 eval;
};

class EvalCache {
public:
    EvalCache();

    void Store(u64 zkey, int eval);
    bool Probe(u64 zkey, int& eval);

    void Clear();

    u64 Size() { return m_size; };
    u64 Occupancy(u64 sampleSize = 1000) const;

private:
    EvalEntry m_evalEntries[EVALCACHE_ENTRIES];

    u64 m_size; // Number of entries
    u64 m_mask;
};

// =====================
// == Pawn-hash entry ==
// =====================

namespace Hash{

    struct PawnEntry {
        u64 zkey;
        i16 evalMg;
        i16 evalEg;
    };
    
    class PawnHash {
    public:
        PawnHash();
        ~PawnHash();
    
        void Store(u64 zkey, int evalMg, int evalEg);
        PawnEntry* Probe(u64 zkey);
    
        void Clear();
    
        int Size() { return PAWN_HASH_SIZE; };
        u64 Occupancy() const;
    
    private:
        PawnEntry* m_pawnEntries;
    };

}

// ======================
// == Global variables ==
// ======================

namespace Hash {
    inline TT tt;
    inline EvalCache evalCache;
    inline PawnHash pawnHash;
}
