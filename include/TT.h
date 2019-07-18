#ifndef TT_H
#define TT_H

#include "Constants.h"
#include "Move.h"

const int MAX_DEPTH = 128;

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

const int HASH_SIZE_MB = 16;
const uint MAX_HASH_ENTRIES = HASH_SIZE_MB * (1024*1024) / sizeof(TTEntry);

#endif //TT_H
