#pragma once

#include "Attacks.h"
#include "NNUE.h"
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
        ZobristKeys::Init();
        NNUE::LoadFile();
    }

    // Silences std::cout while alive
    class MuteCout {
    public:
        MuteCout()  : m_backup(std::cout.rdbuf(nullptr)) {}
        ~MuteCout() { std::cout.rdbuf(m_backup); }

    private:
        std::streambuf* m_backup;
    };

    // Captures std::cout while alive
    class CaptureCout {
    public:
        CaptureCout()  : m_backup(std::cout.rdbuf( m_buffer.rdbuf() )) {}
        ~CaptureCout() { std::cout.rdbuf(m_backup); }

        std::string Str() const { return m_buffer.str(); }

    private:
        std::ostringstream m_buffer;
        std::streambuf* m_backup;
    };

}
