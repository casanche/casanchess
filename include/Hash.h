#ifndef HASH_H
#define HASH_H

#include "Constants.h"
#include "Move.h"

const int MAX_DEPTH = 128;
const uint DEFAULT_HASH_SIZE = 16; //In MegaBytes
const int PAWN_HASH_SIZE = 8192; //In number of entries

// =========================
// == Transposition table ==
// =========================

//Alpha node: the true eval is at most equal to the score (true <= score) UPPER_BOUND
//Beta node: the true eval is at least equal to the score (true >= score) LOWER_BOUND
enum TTENTRY_TYPE { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

//64(zkey) + 32(move) + 16(score) + 8(depth) + 2(type) + 6(age) = 128
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
    void AddEntry(u64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int ply, int age);
    TTEntry* ProbeEntry(u64 zkey, int depth);
    int OccupancyPerMil();
    u64 NumEntries();
    void SetSize(int size);
    u64 Size() { return m_size; };

    int ScoreFromHash(int score, int ply);

private:
    int ScoreToHash(int score, int ply);

    TTEntry* m_entries;
    u64 m_size;
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

namespace Hash {
    extern TT tt;
    extern PawnHash pawnHash;
}

#endif //HASH_H
