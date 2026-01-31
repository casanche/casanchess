#include "Board.h"
#include "Evaluation.h"
#include "NNUE.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

TEST(NNUE, IncrementalMatchesFull) {
    Board board;
    
    // Sequence covering all NNUE update paths:
    // - Quiet moves (Inputs_MovePiece)
    // - Capture (Inputs_RemovePiece)
    // - King move (Inputs_FullUpdate)
    // - Castling (Inputs_FullUpdate)
    board.MakeMove("e2e4"); // quiet
    board.MakeMove("d7d5");
    board.MakeMove("e4d5"); // capture
    board.MakeMove("g8f6");
    board.MakeMove("g1f3");
    board.MakeMove("f6d5"); // capture
    board.MakeMove("f1c4");
    board.MakeMove("e7e6");
    board.MakeMove("e1g1"); // castling
    
    int evalIncremental = Evaluation::Evaluate(board);
    nnue.Inputs_FullUpdate();
    int evalFull = Evaluation::Evaluate(board);
    
    EXPECT_EQ(evalIncremental, evalFull);
}
