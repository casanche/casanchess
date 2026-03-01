#include "Board.h"
#include "Evaluation.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

TEST(NNUE, IncrementalMatchesFull) {
    // Position after: e2e4 d7d5 e4d5 g8f6 g1f3 f6d5 f1c4 e7e6 e1g1
    // Covers all NNUE udpate paths:
    //      Quiet (Inputs_MovePiece)
    //      Capture (Inputs_RemovePiece)
    //      Castling and king move (Inputs_FullUpdate)
    const char* POSITION_FEN = "rnbqkb1r/ppp2ppp/4p3/3n4/2B5/5N2/PPPP1PPP/RNBQ1RK1 b kq - 1 5";

    int evalIncremental;
    {
        Board board;
        board.MakeMove("e2e4"); board.MakeMove("d7d5");
        board.MakeMove("e4d5"); board.MakeMove("g8f6");
        board.MakeMove("g1f3"); board.MakeMove("f6d5");
        board.MakeMove("f1c4"); board.MakeMove("e7e6");
        board.MakeMove("e1g1"); // castling
        evalIncremental = Evaluation::Evaluate(board);
    }

    int evalFull;
    {
        // SetFen triggers full NNUE recompute
        Board board;
        board.SetFen(POSITION_FEN);
        evalFull = Evaluation::Evaluate(board);
    }

    EXPECT_EQ(evalIncremental, evalFull);
}
