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
        tt.SetSize(MIN_HASH_SIZE);
        zkey = 0x123456789ABCDEF0;
        normalScore = 150;
    }
};

TEST_F(TT_Test, ProbeFalse) {
    TTEntry ttEntry;
    ASSERT_FALSE(tt.Probe(zkey, ttEntry));
}

// Mate scores get adjusted by ply
TEST_F(TT_Test, MateScoreAdjustment) {
    int mateIn5 = MATESCORE_MAX - 5;
    tt.Store(zkey, mateIn5, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/3);

    TTEntry ttEntry;
    ASSERT_TRUE(tt.Probe(zkey, ttEntry));

    EXPECT_EQ(tt.ScoreFromHash(ttEntry.score, 3), mateIn5);
    EXPECT_EQ(tt.ScoreFromHash(ttEntry.score, 0), MATESCORE_MAX - 2);
}

// Normal scores should NOT be adjusted
TEST_F(TT_Test, NormalScoreUnchanged) {
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), 10, /*ply=*/5);

    TTEntry ttEntry;
    ASSERT_TRUE(tt.Probe(zkey, ttEntry));

    EXPECT_EQ(tt.ScoreFromHash(ttEntry.score, 0), normalScore);
    EXPECT_EQ(tt.ScoreFromHash(ttEntry.score, 10), normalScore);
}

// Age 6-bit bitfield behaves correctly after overflow (> 63)
TEST_F(TT_Test, AgeBitfieldProtection) {
    // Search number 64. Store a high-depth entry
    for(int i = 0; i < 64; ++i)
        tt.NewSearch();
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/21, /*ply=*/0);

    // Same search. Try to store the same position with a lower-depth. Should NOT overwrite!
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/1, /*ply=*/0);

    TTEntry ttEntry;
    ASSERT_TRUE(tt.Probe(zkey, ttEntry));

    EXPECT_EQ(ttEntry.depth, 21);
}

TEST_F(TT_Test, NullBestMoveIgnored) {
    Move move = Move(G1, F3, KNIGHT, MOVE_TYPE::NORMAL);
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, move, /*depth=*/1, /*ply=*/0);
    tt.Store(zkey, normalScore, TTENTRY_TYPE::EXACT, Move(), /*depth=*/2, /*ply=*/0);

    TTEntry ttEntry;
    ASSERT_TRUE(tt.Probe(zkey, ttEntry));

    EXPECT_EQ(ttEntry.bestMove, move);
}
