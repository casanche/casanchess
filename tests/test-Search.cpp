#include "test-Common.h"

#include "Board.h"
#include "MoveGenerator.h"
#include "Search.h"
#include "Uci.h"

int main(int argc, char** argv) {
    TestCommon::InitEngine();
    UCI_AMBITION = 0;
    UCI_DRAW_CONTEMPT = 0;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class SearchTest : public ::testing::Test {
protected:
    TT tt;
    Search search{tt};
    Board board;
    TestCommon::MuteCout mute;

    void SearchPosition(const char* fen, int depth = 1) {
        board.SetFen(fen);
        tt.Clear();
        search.IterativeDeepening(board, UCI_Limits::FixDepth(depth), true);
    }
};

TEST_F(SearchTest, MateInOne) {
    for(const char* fen : {
        "7k/5Q2/6K1/8/8/8/8/8 w - - 0 1",
        "8/8/8/8/8/6k1/5q2/7K b - - 0 1",
        "7k/5P2/6K1/8/8/8/8/8 w - - 0 1" // Promotion mate
    }) {
        SCOPED_TRACE(fen);
        SearchPosition(fen);
        EXPECT_EQ(search.BestScore(), MATESCORE_MAX - 1);
        ASSERT_FALSE(search.BestMove().IsNull());

        // Accept any mating move, independently of move ordering.
        board.MakeMove(search.BestMove());
        EXPECT_TRUE(board.IsCheck());
        EXPECT_TRUE(MoveGenerator::GenerateMoves(board).empty());
    }
}

TEST_F(SearchTest, MateInTwo) {
    SearchPosition("k7/8/2K5/8/3Q4/8/8/8 w - - 0 1", 6);
    EXPECT_EQ(search.BestScore(), MATESCORE_MAX - 3);
    const std::string move = search.BestMove().Notation();
    EXPECT_TRUE(move == "c6b6" || move == "d4g7" || move == "d4d7"
             || move == "d4b4" || move == "d4b2");
}

TEST_F(SearchTest, ForcedLoss) {
    SearchPosition("7k/4Q3/6K1/8/8/8/8/8 b - - 0 1", 2);
    EXPECT_EQ(search.BestMove().Notation(), "h8g8"); // Only legal move
    EXPECT_EQ(search.BestScore(), -MATESCORE_MAX + 2);
}

TEST_F(SearchTest, Checkmate) {
    for(const char* fen : {
        "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1",
        "8/8/8/8/8/6k1/6q1/7K w - - 0 1"
    }) {
        SCOPED_TRACE(fen);
        SearchPosition(fen);
        EXPECT_EQ(search.BestScore(), -MATESCORE_MAX);
        EXPECT_TRUE(search.BestMove().IsNull());
    }
}

TEST_F(SearchTest, Stalemate) {
    for(const char* fen : {
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
        "8/8/8/8/8/6k1/5q2/7K w - - 0 1"
    }) {
        SCOPED_TRACE(fen);
        SearchPosition(fen);
        EXPECT_EQ(search.BestScore(), 0);
        EXPECT_TRUE(search.BestMove().IsNull());
    }
}

TEST_F(SearchTest, InsufficientMaterial) {
    SearchPosition("8/8/8/8/8/4k3/8/4K3 w - - 0 1", 2);
    EXPECT_EQ(search.BestScore(), 0);
    EXPECT_FALSE(search.BestMove().IsNull());
}
