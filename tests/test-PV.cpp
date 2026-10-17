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
        std::vector<std::string> pv;
        std::string bestmove;
        std::string ponder;
    };

    SearchOutput RunSearch(const UCI_Limits& limits) {
        SearchOutput result;

        // Capture the UCI output
        CaptureCout capture;
        search.IterativeDeepening(board, limits, true);
        std::istringstream output(capture.Str());

        const std::string pvToken = " pv ";
        std::string pvLine;
        for(std::string line; std::getline(output, line); ) {
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
            result.pv.push_back(move);
        return result;
    }
};

TEST_F(PVTest, Mate) {
    board.SetFen("8/8/8/8/4K1Q1/8/7k/8 w - - 0 1");
    const SearchOutput result = RunSearch(UCI_Limits::FixDepth(3));

    EXPECT_EQ(search.BestScore(), MATESCORE_MAX - 3);
    EXPECT_EQ(result.pv.size(), 3);
}

TEST_F(PVTest, Ponder_Stopped) {
    UCI_PONDER = true;

    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(2));
    const int nodes = static_cast<int>(search.GetNodes());

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(nodes + 1)); // Interrupt the search at depth 3 before the first rootmove is completed

    UCI_PONDER = false;

    ASSERT_FALSE(completed.ponder.empty());
    EXPECT_EQ(completed.bestmove, stopped.bestmove);
    EXPECT_EQ(completed.ponder, stopped.ponder);
}
