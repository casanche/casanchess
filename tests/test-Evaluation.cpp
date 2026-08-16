#include "Board.h"
#include "Evaluation.h"
#include "NNUE.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

testing::AssertionResult IsIncrementalCorrect(Board& board) {
    // Evaluate with incremental update (input)
    int evalIncremental = Evaluation::Evaluate(board);

    // Evaluate with full update
    Board cleanBoard;
    cleanBoard.SetFen(board.GetFen());
    int evalFull = Evaluation::Evaluate(cleanBoard);

    if(evalIncremental == evalFull) {
        return testing::AssertionSuccess();
    }

    return testing::AssertionFailure()
        << "NNUE desync! "
        << "FEN " << board.GetFen() << " does not match full update evaluation\n"
        << "Incremental evaluation: " << evalIncremental << "\n"
        << "Full evaluation: " << evalFull;
}

// Sequence covering all NNUE update paths
TEST(NNUE, Incremental_vs_FullUpdate) {
    Board board;

    // Quiet moves
    board.MakeMove("e2e4");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.MakeMove("d7d5");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    // Captures
    board.MakeMove("e4d5");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.MakeMove("c7c6");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.MakeMove("d5c6");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.MakeMove("b8c6");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    // Take moves
    board.TakeMove();
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.TakeMove();
    EXPECT_TRUE(IsIncrementalCorrect(board));

    // King move
    board.MakeMove("e1e2");
    EXPECT_TRUE(IsIncrementalCorrect(board));

    // Null moves
    board.MakeNull();
    EXPECT_TRUE(IsIncrementalCorrect(board));

    board.TakeNull();
    EXPECT_TRUE(IsIncrementalCorrect(board));
}
