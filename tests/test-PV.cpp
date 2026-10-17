#include "Board.h"
#include "Search.h"
#include "Uci.h"

#include "test-Common.h"
using namespace TestCommon;

#include <sstream>
#include <string>
#include <vector>

class PVTest : public ::testing::Test {
protected:
    TT tt;
    Search search{tt};
    Board board;

    struct SearchOutput {
        std::vector<std::string> lines; // full UCI output
        std::vector<std::string> lastPV;
        std::string bestmove;
        std::string ponder;
    };

    void TearDown() override {
        UCI_PONDER = false;
    }

    SearchOutput RunSearch(const UCI_Limits& limits) {
        SearchOutput result;

        // Capture the UCI output
        CaptureCout capture;
        search.IterativeDeepening(board, limits, true);
        std::istringstream output(capture.Str());

        const std::string pvToken = " pv ";
        std::string pvLine;
        for(std::string line; std::getline(output, line); ) {
            // Whole line
            result.lines.push_back(line);

            // Last PV line
            const auto pvStart = line.find(pvToken);
            if(pvStart != std::string::npos)
                pvLine = line.substr(pvStart + pvToken.size());

            // Bestmove and ponder
            if(line.starts_with("bestmove ")) {
                std::istringstream answer(line);
                std::string token;
                answer >> token >> result.bestmove;
                if(answer >> token && token == "ponder")
                    answer >> result.ponder;
            }
        }

        // Split the PV line into single moves
        std::istringstream moves(pvLine);
        for(std::string move; moves >> move; )
            result.lastPV.push_back(move);

        return result;
    }

    // For a given 'info' line, get the value for a field (e.g. get the number of nodes)
    static std::string InfoValue(const std::string& line, const std::string& name) {
        std::istringstream tokens(line);
        for(std::string token; tokens >> token; ) {
            if(token == name && tokens >> token)
                return token;
        }
        return "";
    }
};

TEST_F(PVTest, Mate) {
    board.SetFen("8/8/8/8/4K1Q1/8/7k/8 w - - 0 1");
    const SearchOutput result = RunSearch(UCI_Limits::FixDepth(3));

    EXPECT_EQ(search.BestScore(), MATESCORE_MAX - 3);
    EXPECT_EQ(result.lastPV.size(), 3);
}

// Interrupt the search at next depth before the first rootmove is completed
TEST_F(PVTest, Ponder_Stopped) {
    UCI_PONDER = true;

    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(2));
    const int nodes = static_cast<int>(search.GetNodes());

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(nodes + 1));

    ASSERT_FALSE(completed.ponder.empty());
    EXPECT_EQ(completed.bestmove, stopped.bestmove);
    EXPECT_EQ(completed.ponder, stopped.ponder);

    // The stopped iteration should not print a line
    for(const std::string& line : stopped.lines)
        EXPECT_NE(InfoValue(line, "depth"), "3");
}

// Fail-high in the root + new move
// Bestmove changes and ponder is empty
TEST_F(PVTest, Ponder_FailHigh_NewMove) {
    UCI_PONDER = true;

    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(4));

    int failHighNodes = 0;
    std::string failHighMove;
    std::string previousMove;
    for(const std::string& line : completed.lines) {
        const std::string move = InfoValue(line, "pv");
        if(move.empty()) continue;
    
        if(move != previousMove && line.contains(" lowerbound ")) {
            failHighMove = move;
            failHighNodes = std::stoi( InfoValue(line, "nodes") );
            break;
        }
        previousMove = move;
    }

    if(failHighMove.empty())
        GTEST_SKIP() << "This test needs fail-high at root with a new move";

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(failHighNodes + 1));

    EXPECT_EQ(stopped.bestmove, failHighMove);
    EXPECT_TRUE(stopped.ponder.empty());
}

// Fail-high in the root + same move
// Bestmove and ponder remains the same
TEST_F(PVTest, Ponder_FailHigh_SameMove) {
    UCI_PONDER = true;

    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(6));

    int failHighNodes = 0;
    std::string failHighMove;
    std::string previousMove;
    std::string previousLine;
    for(const std::string& line : completed.lines) {
        const std::string move = InfoValue(line, "pv");
        if(move.empty()) continue;

        const bool previousCompleted = !previousLine.contains("bound ");
        if(move == previousMove && previousCompleted && line.contains(" lowerbound ")) {
            failHighMove = move;
            failHighNodes = std::stoi( InfoValue(line, "nodes") );
            break;
        }
        previousMove = move;
        previousLine = line;
    }

    if(failHighMove.empty())
        GTEST_SKIP() << "This test needs fail-high at root with the same move";

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(failHighNodes + 1));

    EXPECT_EQ(stopped.bestmove, failHighMove);
    EXPECT_FALSE(stopped.ponder.empty());
}
