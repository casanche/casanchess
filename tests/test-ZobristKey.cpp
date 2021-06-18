#include "test-Common.h"

#include "Board.h"
#include "MoveGenerator.h"
#include "ZobristKeys.h"

#include <gtest/gtest.h>

class ZobristKeyTest : public ::testing::Test {
protected:
    Board board;
    Board boardCastling;
    u64 initialKey;
    u64 initialKeyCastling;
    MoveGenerator gen;
    MoveList moves;
    void SetUp() override {
        board.SetFen("5N2/r3nP2/1P5q/8/k1P2Bp1/4p3/1P4pb/n2K4 w - - 0 1");
        boardCastling.SetFen("r1bqkbr1/3p1p2/p1n1p3/1p5p/3pP1p1/1B1P2B1/PPP1NPPP/R2QK2R w KQq - 0 15");
        initialKey = board.ZKey();
        initialKeyCastling = boardCastling.ZKey();
    }
};

TEST_F(ZobristKeyTest, Quiet) {
    board.MakeMove("d1c1");
    EXPECT_NE(initialKey, board.ZKey());
    board.TakeMove();
    EXPECT_EQ(initialKey, board.ZKey());
}

TEST_F(ZobristKeyTest, Capture) {
    board.MakeMove("f4h6");
    EXPECT_NE(initialKey, board.ZKey());
    board.TakeMove();
    EXPECT_EQ(initialKey, board.ZKey());
}

TEST_F(ZobristKeyTest, PromotionCapture) {
    board.SetFen("r4N2/1P2nP2/7q/8/1kP2Bp1/4p3/1P4pb/n2K4 w - - 0 1");
    initialKey = board.ZKey();
    board.MakeMove("b7a8q");
    EXPECT_NE(initialKey, board.ZKey());
    board.TakeMove();
    EXPECT_EQ(initialKey, board.ZKey());
}

TEST_F(ZobristKeyTest, Color) {
    board.SetFen("5N2/r3nP2/1P5q/8/k1P2Bp1/4p3/1P4pb/n2K4 b - - 0 1");
    u64 blackKey = board.ZKey();
    EXPECT_NE(initialKey, blackKey);
    board.SetFen("5N2/r3nP2/1P5q/8/k1P2Bp1/4p3/1P4pb/n2K4 w - - 0 1");
    EXPECT_EQ(initialKey, board.ZKey());

    //King triangulation
    board.MakeMove("d1e1");
    board.MakeMove("a4b4");
    board.MakeMove("e1e2");
    board.MakeMove("b4a4");
    board.MakeMove("e2d1");
    EXPECT_EQ(board.ZKey(), blackKey);
}

TEST_F(ZobristKeyTest, Castling) {
    boardCastling.MakeMove("e1g1");
    EXPECT_NE(initialKeyCastling, boardCastling.ZKey());
    boardCastling.TakeMove();
    EXPECT_EQ(initialKeyCastling, boardCastling.ZKey());
}

TEST_F(ZobristKeyTest, ZobristAfterPerft) {
    board.Perft(3);
    EXPECT_EQ(initialKey, board.ZKey());

    boardCastling.Perft(3);
    EXPECT_EQ(initialKeyCastling, boardCastling.ZKey());
}
