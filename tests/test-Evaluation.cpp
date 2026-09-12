#include "Board.h"
#include "Evaluation.h"
#include "NNUE.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

testing::AssertionResult IsIncrementalCorrect(Board& board) {
    // Evaluate with incremental update (input)
    const EvaluationOutput incremental = Evaluation::EvaluateOutputs(board);

    // Evaluate with full update
    Board cleanBoard;
    cleanBoard.SetFen(board.GetFen());
    const EvaluationOutput full = Evaluation::EvaluateOutputs(cleanBoard);

    if(incremental.eval == full.eval && incremental.drawishness == full.drawishness) {
        return testing::AssertionSuccess();
    }

    return testing::AssertionFailure()
        << "NNUE desync! "
        << "FEN " << board.GetFen() << " does not match full update evaluation\n"
        << "Incremental output: " << incremental.eval << ", " << incremental.drawishness << "\n"
        << "Full output: " << full.eval << ", " << full.drawishness;
}

void ExpectCombinedOutputMatchesIndividualHeads(const Board& board) {
    const EvaluationOutput nnue = board.NNUE_EvaluateOutputs();
    EXPECT_EQ(nnue.eval, board.NNUE_Evaluate());
    EXPECT_EQ(nnue.drawishness, board.NNUE_Drawishness());

    const EvaluationOutput output = Evaluation::EvaluateOutputs(board);
    EXPECT_EQ(output.eval, Evaluation::Evaluate(board));
    EXPECT_EQ(output.eval, nnue.eval);
    EXPECT_EQ(output.drawishness, nnue.drawishness);
}

// Sequence covering all NNUE update paths
TEST(NNUE, Incremental_vs_FullUpdate) {
    Board board;
    ExpectCombinedOutputMatchesIndividualHeads(board);

    // Quiet moves
    board.MakeMove("e2e4");
    EXPECT_TRUE(IsIncrementalCorrect(board));
    ExpectCombinedOutputMatchesIndividualHeads(board);

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
    ExpectCombinedOutputMatchesIndividualHeads(board);

    board.TakeNull();
    EXPECT_TRUE(IsIncrementalCorrect(board));
    ExpectCombinedOutputMatchesIndividualHeads(board);
}
