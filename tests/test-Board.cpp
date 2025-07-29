#include "Board.h"
#include "Evaluation.h"

#include <fstream>

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

TEST(FenTest, EnPassant) {
    Board board;
    board.SetFen("rnbqkbnr/p1pppppp/8/8/1Pp5/2N5/P2PPPPP/R1BQKBNR b KQkq b3");
    EXPECT_EQ(board.EnPassantSquare(), SquareBB(SQUARES::B3));
}

TEST(BoardTest, IsRepetitionDraw) {
    Board board;
    board.SetFen("8/p5pp/1r2bpk1/8/2P1P3/q1P2PQ1/PR4PP/2KR4 b - - 5 32");
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h4g3"); EXPECT_EQ(board.IsRepetitionDraw(), false); //1st repetition
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h4g3"); EXPECT_EQ(board.IsRepetitionDraw(), true); //2nd repetition
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
}

#include "Uci.h"

TEST(EvaluationTest, Mirror) {
    Board board;
    UCI_CLASSICAL_EVAL = true; // NNUE eval does not respect mirroring

    board.SetFen("r4rk1/pppbqppp/2n1pn2/1B1p4/3P4/P1B1P3/1PPN1PPP/R2Q1RK1 b - - 0 10"); // RG3
    // Mirrored: "r2q1rk1/1ppn1ppp/p1b1p3/3p4/1b1P4/2N1PN2/PPPBQPPP/R4RK1 w - - 0 10"

    int eval = Evaluation::Evaluate(board);

    board.Mirror();
    int evalMirror = Evaluation::Evaluate(board);

    UCI_CLASSICAL_EVAL = false;
    EXPECT_EQ(eval, evalMirror);
}
