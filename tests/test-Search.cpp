#include "Board.h"
#include "Search.h"

#include <gtest/gtest.h>

#include <iostream>

class ShortSearch : public ::testing::Test {
protected:
    Search search;
    Board board;
    int depth = 7; //short: 5, long: 7
    int time = 1000 + (depth > 5)*10000;
    void SetUp() override {
        std::cout.rdbuf(nullptr);
        search.FixDepth(depth);
    }
};

TEST_F(ShortSearch, Start) {
    // board.SetFen(STARTFEN);
    search.FixDepth(depth+1);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// RG = Position from Real Game
TEST_F(ShortSearch, RG1) {
    board.SetFen("r2qkb2/1b1p1pp1/p1n1p3/1pp4r/4PB2/1BNP2QP/PPP2P1P/R3R1K1 w q - 3 16");
    search.FixDepth(depth-2);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Nd5
TEST_F(ShortSearch, RG2_Nd5) {
    board.SetFen("r2qkb2/1b1p1pp1/p1n1p3/2p4r/Pp2PB2/1BNP2QP/1PP2P1P/R3R1K1 w q - 0 17");
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
//Failed with Rad8 (order better the root moves)
TEST_F(ShortSearch, RG3) {
    board.SetFen("r4rk1/pppbqppp/2n1pn2/1B1p4/3P4/P1B1P3/1PPN1PPP/R2Q1RK1 b - - 0 10");
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
//Failed with Rc1, losing a bishop
TEST_F(ShortSearch, RG4) {
    board.SetFen("8/5p2/2B2k2/p7/1pr5/4r3/P1P3P1/3R2K1 w - - 12 48");
    search.FixDepth(depth+1);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
//Test passed pawn, Rd4+, most engines realize at depth 6/7
TEST_F(ShortSearch, RG5_Rd4) {
    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    search.FixDepth(depth+2);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
//Crowdy early-final position
TEST_F(ShortSearch, RG6_crowdy) {
    board.SetFen("8/8/1p1rknp1/pBb5/2P1nP1p/P1N4P/1BP3K1/4R3 b - - 14 41");
    search.FixDepth(depth+2);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Failed with Rc8, depth 8, 8s
TEST_F(ShortSearch, RG7) {
    board.SetFen("4r1k1/1p3ppp/b7/3R4/1pn5/4P3/3N1PPP/R5K1 b - - 0 30");
    search.FixDepth(depth+1);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// bm Qf3, failed to spot at depth 11, 2min
TEST_F(ShortSearch, RG8_Qf3) {
    board.SetFen("2kr2nr/n6p/p3R3/1p3P2/P2q4/2p5/2PN2P1/1RBQ3K w - - 0 26");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// bm b5, not seen until depth 9 (might be related to mate detection)
TEST_F(ShortSearch, RG9_b5) {
    board.SetFen("8/p1r2kp1/1p2bq2/2r2p2/QP2p3/P5PP/2PR1PB1/1K1R4 b - - 0 35");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Failed with fxg5, depth 10, 4s
TEST_F(ShortSearch, RG10_F_fxg5) {
    board.SetFen("6r1/1ppnr1kp/pq1b1p2/2p1pPP1/P3P3/1P1P1N2/R1PB3Q/5R1K b - - 2 30");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Failed with g3 (light squares weakness)
TEST_F(ShortSearch, RG11_F_g3) {
    board.SetFen("r3r1k1/1p3ppp/2nQ4/4Nb2/p4P1q/2P5/PPP3PP/R1BR2K1 w - - 5 18");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Mis-evaluated endgame with -0.14 d18/3s
TEST_F(ShortSearch, RG12_endgame) {
    board.SetFen("5b2/p1p2p2/7p/4kP2/PpPp2P1/5P2/4K3/4R3 b - - 6 36");
    search.FixDepth(depth+3);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Be careful with king safety! Failed with Bd2 0.13s
TEST_F(ShortSearch, RG13_kingSafetyI) {
    board.SetFen("5r1bk2r1/ppp2p2/3p1b1p/1Q1P4/2P1P2q/2N2R2/PP2B1nP/R1B4K w - - 2 15");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Be careful with king safety! Failed with Nxc5 +0.49 d12/5s
TEST_F(ShortSearch, RG14_kingSafetyII) {
    board.SetFen("r2qk2r/pp3ppp/n1p5/2b2b2/Q1P1N1n1/5N2/PP1PBPPP/R1B1K2R w KQkq - 8 10");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Weird king move. Kf1 +0.19 d14/5s
TEST_F(ShortSearch, RG15_weirdMove) {
    board.SetFen("rnb1kbnr/ppp1pppp/6q1/8/8/5N2/PPPPBPPP/RNBQK2R w KQkq - 4 5");
    search.FixDepth(depth);
    search.IterativeDeepening(board);
    EXPECT_LE(search.ElapsedTime(), time);
}
// Other engine eval raise. c5 +0.12 d13/3s
//r1bqr1k1/ppp2ppp/3p1n2/4n3/2PNP3/2P2P2/P3B1PP/1RBQ1RK1 b - - 1 11