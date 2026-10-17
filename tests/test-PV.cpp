#include "Board.h"
#include "Search.h"

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

    std::vector<std::string> SearchPV(const UCI_Limits& limits) {
        // Capture the UCI output
        CaptureCout capture;
        search.IterativeDeepening(board, limits, true);
        std::istringstream output(capture.Str());

        // PV line of the last output
        const std::string pvToken = " pv ";
        std::string pvLine;
        for(std::string line; std::getline(output, line); ) {
            const auto pvStart = line.find(pvToken);
            if(pvStart != std::string::npos)
                pvLine = line.substr(pvStart + pvToken.size());
        }

        // Split the PV line into single moves
        std::vector<std::string> pv;
        std::istringstream moves(pvLine);
        for(std::string move; moves >> move; )
            pv.push_back(move);
        return pv;
    }
};

TEST_F(PVTest, Mate) {
    board.SetFen("8/8/8/8/4K1Q1/8/7k/8 w - - 0 1");
    const auto pv = SearchPV(UCI_Limits::FixDepth(3));

    EXPECT_EQ(search.BestScore(), MATESCORE_MAX - 3);
    EXPECT_EQ(pv.size(), 3);
}
