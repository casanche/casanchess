#pragma once

#include "Constants.h"
#include "Move.h"

#include <memory>

constexpr int MIN_HASH_SIZE = 1; //In MegaBytes
constexpr int DEFAULT_HASH_SIZE = 16; //In MegaBytes
// 2^19 entries = 4 MB / 64 bits per entry
constexpr u64 EVALCACHE_ENTRIES = 1 << 19;

constexpr int TT_AGE_BITS = 6; // 6-bit search counter
constexpr int TT_AGE_CYCLE = 1 << TT_AGE_BITS; // 2^6 = 64

// =========================
// == Transposition table ==
// =========================

// Alpha node (from a fail-low): the true eval is at most equal to the score (truth <= score) UPPER_BOUND
// Beta node (from a fail-high): the true eval is at least equal to the score (truth >= score) LOWER_BOUND
enum class TTENTRY_TYPE : u8 { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

// 32(zkey) + 32(move) + 16(score) + 16(eval) + 8(depth) + 8(wasPV) + 6(age) + 2(type) + 8(padding) = 128 bits per entry
struct TTEntry {
    u32 zkey;
    i16 score;
    i16 eval;
    u8 depth;
    bool wasPV;
    u8 padding;
    TTENTRY_TYPE type : 2;
    u8 age : TT_AGE_BITS;
    Move bestMove;

    void Clear();
};
static_assert(sizeof(TTEntry) == 16, "TTEntry must be 16 bytes.");

// 2^N entries * 16 bytes
// Max N = 32, so the zkey lower 32 bits (TT index) do not overlap with the upper 32 bits (stored zkey)
constexpr int MAX_HASH_SIZE = (u64(1) << 32) * sizeof(TTEntry) / (1024 * 1024); //In MegaBytes

class TT {
public:
    TT();
    ~TT();

    // Avoid copying
    TT(const TT&) = delete;
    TT& operator=(const TT&) = delete;

    void Store(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, bool wasPV, int eval = NO_EVAL);
    bool Probe(u64 zkey, TTEntry& entry) const;

    void NewSearch();

    void Clear();
    void SetSize(int sizeInMB);

    u64 Size() { return m_size; };
    u64 Occupancy(u64 sampleSize = 1000) const;
    static int ScoreFromHash(int score, int ply);

private:
    static int ScoreToHash(int score, int ply);

    TTEntry* m_entries;
    u64 m_size; // Number of entries
    u64 m_mask; // Mask for AND operations
    u8 m_age = 0; // Search counter
};

// ================
// == Eval cache ==
// ================

// 32(zkey) + 16(eval) + 16(padding) = 64 bits per entry
struct EvalEntry {
    u32 zkey;
    i16 eval;
    i16 padding;
};
static_assert(sizeof(EvalEntry) == 8, "EvalEntry must be 8 bytes.");

class EvalCache {
public:
    EvalCache();

    void Store(u64 zkey, int eval);
    bool Probe(u64 zkey, int& eval) const;

    void Clear();

    u64 Size() { return m_size; };
    u64 Occupancy(u64 sampleSize = 1000) const;

private:
    std::unique_ptr<EvalEntry[]> m_evalEntries = std::make_unique<EvalEntry[]>(EVALCACHE_ENTRIES);

    u64 m_size; // Number of entries
    u64 m_mask;
};
