#include "test-Common.h"

#include "Board.h"
#include "ZobristKeys.h"

#include <gtest/gtest.h>

class ZobristKeyTest : public ::testing::Test {
protected:
    Board board;
    Board boardCastling;
    u64 initialKey;
    u64 initialKeyCastling;
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

TEST_F(ZobristKeyTest, EnPassant_Fen) {
    board.SetFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    u64 keyNo = board.ZKey();

    board.SetFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    u64 keyYes = board.ZKey();

    EXPECT_NE(keyNo, keyYes);
}

TEST_F(ZobristKeyTest, EnPassant_MakeMove) {
    Board newBoard; //initial position
    u64 keyBefore = newBoard.ZKey();

    newBoard.MakeMove("e2e4");
    newBoard.TakeMove();

    u64 keyAfter = newBoard.ZKey();

    EXPECT_EQ(keyBefore, keyAfter);
}

TEST_F(ZobristKeyTest, SetFen_vs_MakeMove) {
    // Position after 1. e4 from FEN
    board.SetFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    u64 zkey_fromFen = board.ZKey();

    // Same position after MakeMove
    Board newBoard;
    newBoard.MakeMove("e2e4");
    u64 zkey_fromMakeMove = newBoard.ZKey();

    EXPECT_EQ(zkey_fromFen, zkey_fromMakeMove);
}

TEST_F(ZobristKeyTest, AfterPerft) {
    board.Perft(3);
    EXPECT_EQ(initialKey, board.ZKey());

    boardCastling.Perft(3);
    EXPECT_EQ(initialKeyCastling, boardCastling.ZKey());
}

TEST_F(ZobristKeyTest, HashConsistency) {
    Board board1;
    board1.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    u64 zkey1 = board1.ZKey();
    
    // Run the same position multiple times
    Board board2;
    for (int i = 0; i < 10; i++) {
        board2.SetFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        u64 zkey2 = board2.ZKey();
        EXPECT_EQ(zkey1, zkey2) << "Hash inconsistent at iteration " << i;
    }
}
