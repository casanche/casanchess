#include "Search.h"
#include <iostream>

#include "test-Common.h"
using namespace Test;
#include <gtest/gtest.h>

class PositionMisc : public ::testing::Test {
protected:
    Board board;
    Search search;
    CoutHelper verbosity;
    void SetUp() override {
        verbosity.Mute();
    }
    void TearDown() override {
        verbosity.Speak();
    }
};

TEST_F(PositionMisc, Fine70) {
    board.SetFen("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -");
    search.FixDepth(26);
    search.IterativeDeepening(board);
    EXPECT_EQ(search.BestMove().Notation(), "a1b1");
}

//Mate tests
// TEST_F(PositionMisc, Mate1) {
//     board.SetFen("8/3N4/6p1/1p5p/5b1p/P2k2n1/1B6/3KQ3 w - -");
//     search.FixDepth(10);
//     search.IterativeDeepening(board);
//     EXPECT_EQ(search.BestMove().Notation(), "b2a1");
//     EXPECT_LE(search.ElapsedTime(), 1000);
// }

// 8/3N4/6p1/1p5p/5b1p/P2k2n1/1B6/3KQ3 w - - acn 138553; acs 0; bm Ba1; ce 32758; dm 5; pv Ba1 h3 Nf6 b4 Qe6 Bd2 Qd6+ Ke3 Qxg3#;
// 2RR2bK/b1N4p/1pkNp1r1/p3p2r/6P1/n1p5/n5P1/6B1 w - - acn 330710; acs 1; bm g3; ce 32758; dm 5; pv g3 h6 Ncb5+ Kd5 Nxc3+ Nxc3 Nc4+ Ke4 Nd2#;
// 6RQ/6pP/p5K1/1r2p2N/p3k3/R7/B7/B7 w - - acn 106602; acs 0; bm Qxg7 Rc3; ce 32758; dm 5; pv Rc3 Rb6+ Kg5 Rg6+ Kxg6 a3 Rd8 a5 Bb1#;
