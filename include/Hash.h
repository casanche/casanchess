#ifndef HASH_H
#define HASH_H

#include "Constants.h"
#include "Move.h"

const int MAX_DEPTH = 128;
const uint DEFAULT_HASH_SIZE = 16; //In MegaBytes
const int PAWN_HASH_SIZE = 8192; //In number of entries

// 2 MB / 64 bits per entry = 2^18 entries
constexpr uint EVALCACHE_ENTRIES = 1 << 18;

// =========================
// == Transposition table ==
// =========================

// Alpha node: the true eval is at most equal to the score (true <= score) UPPER_BOUND
// Beta node: the true eval is at least equal to the score (true >= score) LOWER_BOUND
enum TTENTRY_TYPE { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

// 64(zkey) + 32(move) + 16(score) + 8(depth) + 2(type) + 6(age) = 128 bits per entry
struct TTEntry {
    u64 zkey;
    i16 score;
    u8 depth;
    u8 type: 2, age: 6;
    Move bestMove;

    void Clear();
};

class TT {
public:
    TT();
    ~TT();
    void Clear();
    void SetSize(int size);

    void AddEntry(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age); // TODO: rename
    TTEntry* ProbeEntry(u64 zkey, int depth); // TODO: rename

    int OccupancyPerMil();
    u64 NumEntries(); // TODO: rename
    u64 Size() { return m_size; };

    int ScoreFromHash(int score, int ply);

private:
    int ScoreToHash(int score, int ply);

    TTEntry* m_entries;
    u64 m_size; // Number of entries
};

// ================
// == Eval cache ==
// ================

// 32(zkey) + 16(eval) + 16(padding) = 64 bits per entry
struct EvalEntry {
    u32 zkey32;
    i16 eval;

    void Clear();
};

class EvalCache {
public:
    EvalCache();
    void Clear();

    void Store(u64 zkey, int eval);
    bool Probe(u64 zkey, int& eval);

    int OccupancyPerMil();

    u64 Size() { return m_size; };

private:
    EvalEntry m_evalEntries[EVALCACHE_ENTRIES];

    u64 m_size; // Number of entries
    u64 m_mask;
};

// =====================
// == Pawn-hash entry ==
// =====================

struct PawnEntry {
    u64 zkey;
    i16 evalMg;
    i16 evalEg;

    void Clear();
};

class PawnHash {
public:
    PawnHash();
    ~PawnHash();
    void Clear();
    void AddEntry(u64 zkey, int evalMg, int evalEg);
    float Occupancy();
    PawnEntry* ProbeEntry(u64 zkey);
private:
    PawnEntry* m_pawnEntries;
};

// ======================
// == Global variables ==
// ======================

namespace Hash {
    extern TT tt;
    extern EvalCache evalCache;
    extern PawnHash pawnHash;
}

#endif //HASH_H
