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

        for(std::string line; std::getline(output, line); ) {
            // Whole line
            result.lines.push_back(line);

            // Last PV line
            const auto pv = InfoPV(line);
            if(!pv.empty())
                result.lastPV = pv;

            // Bestmove and ponder
            if(line.starts_with("bestmove ")) {
                result.bestmove = FieldValue(line, "bestmove");
                result.ponder = FieldValue(line, "ponder");
            }
        }

        return result;
    }

    // For a given UCI line, get the value for a field (e.g. get the number of nodes)
    static std::string FieldValue(const std::string& line, const std::string& name) {
        std::istringstream tokens(line);
        for(std::string token; tokens >> token; ) {
            if(token == name && tokens >> token)
                return token;
        }
        return "";
    }

    // For a given UCI 'info' line, get the PV moves
    static std::vector<std::string> InfoPV(const std::string& line) {
        std::vector<std::string> pv;

        const std::string pvToken = " pv ";
        const auto pvStart = line.find(pvToken);
        if(pvStart == std::string::npos)
            return pv;

        std::istringstream moves(line.substr(pvStart + pvToken.size()));
        for(std::string move; moves >> move; )
            pv.push_back(move);

        return pv;
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

    // The stopped iteration should not print a PV
    for(const std::string& line : stopped.lines) {
        if(!InfoPV(line).empty()) {
            EXPECT_NE(FieldValue(line, "depth"), "3");
        }
    }
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
        const auto pv = InfoPV(line);
        if(pv.empty()) continue;
        const std::string& move = pv[0];

        const bool failHigh = line.contains(" lowerbound ") && pv.size() == 1;
        if(failHigh && move != previousMove) {
            failHighMove = move;
            failHighNodes = std::stoi( FieldValue(line, "nodes") );
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
// Bestmove remains the same and ponder is available
TEST_F(PVTest, Ponder_FailHigh_SameMove) {
    UCI_PONDER = true;

    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(6));

    int failHighNodes = 0;
    std::string failHighMove;
    std::string previousMove;
    std::string previousLine;
    for(const std::string& line : completed.lines) {
        const auto pv = InfoPV(line);
        if(pv.empty()) continue;
        const std::string& move = pv[0];

        const bool failHigh = line.contains(" lowerbound ") && pv.size() == 1;

        const bool previousCompleted = !previousLine.contains("bound ");
        if(failHigh && move == previousMove && previousCompleted) {
            failHighMove = move;
            failHighNodes = std::stoi( FieldValue(line, "nodes") );
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
