#include "Board.h"
#include "Evaluation.h"

#include <fstream>

#include "test-Common.h"
using namespace Test;
#include <gtest/gtest.h>

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

TEST(EvaluationTest, Mirror) {
    Board board;
    std::ifstream in("../tests/suites/real_games.epd");
    std::string line;

    int count = 0;
    while( std::getline(in, line) ) {
        count++;
        EPDPosition pos = ReadEPDLine(line);

        board.SetFen(pos.fen);
        int eval = Evaluation::Evaluate(board);
        board.Mirror();
        int evalMirror = Evaluation::Evaluate(board);
        EXPECT_EQ(eval, evalMirror);
    }
}
