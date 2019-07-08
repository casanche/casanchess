#ifndef _TT_H
#define _TT_H

#include "Constants.h"
#include "Move.h"

const int MAX_DEPTH = 250;

//24-bytes struct: 262144, 524288, 1048576 (24 MB), 2097152 (48 MB)
const int HASH_SIZE_MB = 24;
const uint MAX_HASH_ENTRIES = HASH_SIZE_MB * (1024*1024) / 24;

//Alpha node: the true eval is at most equal to the score (true <= score) UPPER_BOUND
//Beta mode: the true eval is at least equal to the score (true >= score) LOWER_BOUND
enum TTENTRY_TYPE { NONE, EXACT, LOWER_BOUND, UPPER_BOUND };

//64(zkey) + 32(move) + 4(type) + 16(score) + 8(depth) = 128
struct TTEntry {
    U64 zkey;
    int score;
    TTENTRY_TYPE type;
    Move bestMove;
    U8 depth;
    
    // U8 type : 2;
    // U8 depth : 6;

    void Clear();
};

class TT {
public:
    TT();
    ~TT();
    void AddEntry(U64 zkey, int score, TTENTRY_TYPE type, Move bestMove, int depth, int age);
    void Clear();
    TTEntry* ProbeEntry(U64 zkey, int depth);
    U64 NumEntries();
private:

    TTEntry* m_entries;
};

#endif //_TT_H
