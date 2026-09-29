#include "Uci.h"
#include "Utils.h"

#include "test-Common.h"
using namespace TestCommon;
#include <gtest/gtest.h>

#include <sstream>
#include <string>

class UCI : public ::testing::Test {
protected:
    MuteCout mute;

    // Simulates a UCI session. Returns the duration (ms)
    i64 RunUci(const std::string& commands) {
        std::istringstream input(commands);

        Uci uci({});
        Utils::Clock clock;
        uci.Launch(input);

        return clock.Elapsed();
    }
};

TEST_F(UCI, RaceCondition_GoStop) {
    std::string commands;
    for(int i = 0; i < 20; i++)
        commands += "go movetime 1000\nstop\n";

    EXPECT_LT(RunUci(commands), 1000);
}
