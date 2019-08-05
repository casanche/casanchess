#ifndef HASH_H
#define HASH_H

#include "Constants.h"
#include "Move.h"

const int MAX_DEPTH = 128;

// =========================
// == Transposition table ==
// =========================

//Alpha node: the true eval is at most equal to the score (true <= score) UPPER_BOUND
//Beta node: the true eval is at least equal to the score (true >= score) LOWER_BOUND
enum TTENTRY_TYPE { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

//64(zkey) + 32(move) + 16(score) + 8(depth) + 2(type) + 6(age) = 128
struct TTEntry {
    U64 zkey;
    I16 score;
    U8 depth;
    U8 type: 2, age: 6;
    Move bestMove;

    void Clear();
};

const int HASH_SIZE_MB = 16;
constexpr uint MAX_HASH_ENTRIES = HASH_SIZE_MB * (1024*1024) / sizeof(TTEntry);

class TT {
public:
    TT();
    ~TT();
    void Clear();
    void AddEntry(U64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int age);
    TTEntry* ProbeEntry(U64 zkey, int depth);
    int OccupancyPerMil();
    U64 NumEntries();
private:

    TTEntry* m_entries;
};

// =====================
// == Pawn-hash entry ==
// =====================
const int PAWN_HASH_SIZE = 8192;

struct PawnEntry {
    U64 zkey;
    I16 evalMg;
    I16 evalEg;

    void Clear();
};

class PawnHash {
public:
    PawnHash();
    ~PawnHash();
    void Clear();
    void AddEntry(U64 zkey, int evalMg, int evalEg);
    PawnEntry* ProbeEntry(U64 zkey);
private:
    PawnEntry* m_pawnEntries;
};

namespace Hash {
    extern PawnHash pawnHash;
}

#endif //HASH_H
