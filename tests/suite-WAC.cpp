#include "Board.h"
#include "Search.h"
#include <fstream>

#include "test-Common.h"
using namespace Test;

#include <gtest/gtest.h>

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
        search.IterativeDeepening(board);
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
