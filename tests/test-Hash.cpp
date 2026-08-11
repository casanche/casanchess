#include "Hash.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

TEST(TT, MateScoreAdjustment) {
    TT tt;
    tt.SetSize(1);
    u64 zkey = 0x123456789ABCDEF0;

    // Mate scores get adjusted by ply
    int mateIn5 = MATESCORE_MAX - 5;
    tt.Store(zkey, mateIn5, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/3, 0);
    TTEntry* entry = tt.Probe(zkey);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 3), mateIn5);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 0), MATESCORE_MAX - 2);
}

TEST(TT, NormalScoreUnchanged) {
    TT tt;
    tt.SetSize(1);
    u64 zkey = 0xABCDEF0123456789;

    // Normal scores should NOT be adjusted
    int normalScore = 150;
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/5, 0);
    TTEntry* entry = tt.Probe(zkey);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 0), normalScore);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 10), normalScore);
}
