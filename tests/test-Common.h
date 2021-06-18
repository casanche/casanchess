#ifndef _TEST_COMMON
#define _TEST_COMMON

#include "Attacks.h"
#include "Evaluation.h"
#include "ZobristKeys.h"

#include <iostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

//https://github.com/google/googletest/blob/master/googletest/docs/primer.md
//ASSERT_* generate fatal failures, EXPECT_* generate nonfatal failures (preferred)
//EXPECT_TRUE, EXPECT_FALSE
//EXPECT_EQ, EXPECT_NE, EXPECT_LT, EXPECT_LE, EXPECT_GT, EXPECT_GE
//EXPECT_STREQ, EXPECT_STRNE, EXPECT_STRCASEEQ, EXPECT_STRCASENE

namespace TestCommon {

    inline void InitEngine() {
        Attacks::Init();
        Evaluation::Init(); //after Attacks
        ZobristKeys::Init();
    }

    class CoutHelper {
    public:
        CoutHelper() {
            m_backup = std::cout.rdbuf();
        }
        void Mute() {
            std::cout.rdbuf(nullptr);
        }
        void Speak() {
            std::cout.rdbuf(m_backup);
        }
    private:
        std::streambuf* m_backup;
    };

    struct EPDPosition {
        std::string fen;
        std::string bestMove;
        std::string id;
    };

    EPDPosition ReadEPDLine(std::string line);

}

#endif //_TEST_COMMON
