#include "Board.h"
#include "Search.h"
#include <fstream>

#include "test-Common.h"
using namespace TestCommon;

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    TestCommon::InitEngine();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class MySuite : public ::testing::Test {
protected:
    Search search;
    Board board;
    void SetUp() override {
        std::cout.rdbuf(nullptr);
    }
};

TEST_F(MySuite, Fine70) {
    board.SetFen("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -");
    search.FixTime(3000);
    search.IterativeDeepening(board, true);
    EXPECT_EQ(search.BestMove().Notation(), "a1b1");
}

TEST(WAC, WAC) {
    CoutHelper coutHelper;
    coutHelper.Mute();

    Board board;
    Search search;
    std::ifstream in("../tests/suites/wac.epd");
    std::string line;
    int totalCount = 0, successCount = 0;

    while( std::getline(in, line) ) {
        EPDPosition pos = ReadEPDLine(line);

        board.SetFen(pos.fen);

        search.FixTime(3000);
        search.IterativeDeepening(board, true);
        Move move = search.BestMove();

        std::string toSq = move.Notation().substr(2,2);

        // EXPECT_NE(pos.bestMove.find(toSq), std::string::npos);

        ++totalCount;
        if(pos.bestMove.find(toSq) != std::string::npos) {
            ++successCount;
        } else {
            coutHelper.Speak();
            std::cout << std::endl << pos.id << " failed" << std::endl;
            coutHelper.Mute();
        }
        
        coutHelper.Speak();
        std::cout << "\r" << successCount << "/" << totalCount << std::flush;
        coutHelper.Mute();
    }

    EXPECT_GE(successCount, 190);
}
