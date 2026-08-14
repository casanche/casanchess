#include "Hash.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

class TT_Test : public ::testing::Test {
protected:
    TT tt;
    u64 zkey;
    int normalScore;

    void SetUp() override {
        tt.SetSize(1);
        zkey = 0x123456789ABCDEF0;
        normalScore = 150;
    }
};

// Mate scores get adjusted by ply
TEST_F(TT_Test, MateScoreAdjustment) {
    int mateIn5 = MATESCORE_MAX - 5;
    tt.Store(zkey, mateIn5, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/3, 0);

    TTEntry* entry = tt.Probe(zkey);
    ASSERT_NE(entry, nullptr);

    EXPECT_EQ(tt.ScoreFromHash(entry->score, 3), mateIn5);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 0), MATESCORE_MAX - 2);
}

// Normal scores should NOT be adjusted
TEST_F(TT_Test, NormalScoreUnchanged) {
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/5, 0);

    TTEntry* entry = tt.Probe(zkey);
    ASSERT_NE(entry, nullptr);

    EXPECT_EQ(tt.ScoreFromHash(entry->score, 0), normalScore);
    EXPECT_EQ(tt.ScoreFromHash(entry->score, 10), normalScore);
}

// Age 6-bit bitfield behaves correctly after overflow (> 63)
TEST_F(TT_Test, AgeBitfieldProtection) {
    // SearchCount reached 64. Store a high-depth entry
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/21, /*ply=*/0, /*age=*/64);

    // Same search. Try to store the same position with a lower-depth. Should NOT overwrite!
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/1, /*ply=*/0, /*age=*/64);

    TTEntry* entry = tt.Probe(zkey);
    ASSERT_NE(entry, nullptr);

    EXPECT_EQ(entry->depth, 21);
}

TEST_F(TT_Test, NullBestMoveIgnored) {
    Move move = Move(G1, F3, KNIGHT, MOVE_TYPE::NORMAL);
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, move, /*depth=*/1, /*ply=*/0, /*age=*/0);
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/2, /*ply=*/0, /*age=*/0);

    TTEntry* entry = tt.Probe(zkey);
    ASSERT_NE(entry, nullptr);

    EXPECT_EQ(entry->bestMove, move);
}
