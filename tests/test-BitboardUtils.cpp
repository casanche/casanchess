#include "BitboardUtils.h"

#include <gtest/gtest.h>

//https://github.com/google/googletest/blob/master/googletest/docs/primer.md
//ASSERT_* generate fatal failures, EXPECT_* generate nonfatal failures (preferred)
//EXPECT_TRUE, EXPECT_FALSE
//EXPECT_EQ, EXPECT_NE, EXPECT_LT, EXPECT_LE, EXPECT_GT, EXPECT_GE
//EXPECT_STREQ, EXPECT_STRNE, EXPECT_STRCASEEQ, EXPECT_STRCASENE

//BitboardUtils
TEST(BitboardUtils, GetBit) {
    EXPECT_FALSE( GetBit(0, 0) );
    EXPECT_FALSE( GetBit(0, 50) );
    EXPECT_FALSE( GetBit(0x9102289248220842, 16) );
    EXPECT_TRUE( GetBit(0x9102289248220842, 63) );
}
TEST(BitboardUtils, BitscanForward) {
    EXPECT_EQ(BitscanForward(0), -1); //expected behaviour for empty bitboards
    EXPECT_EQ(BitscanForward(0x4000000000010), 4); //Bit index: 4 and 50
    EXPECT_EQ(BitscanForward(0x4000000000000), 50); //Bit index: 50
}
TEST(BitboardUtils, BitscanReverse) {
    EXPECT_EQ(BitscanReverse(0), -1);
    EXPECT_EQ(BitscanReverse(0x4000000000010), 50); //Bit index: 4 and 50
    EXPECT_EQ(BitscanReverse(0x10), 4); //Bit index: 4
}
TEST(BitboardUtils, PopCount) {
    EXPECT_EQ(PopCount(0),0);
    EXPECT_EQ(PopCount(0x4000000000010),2);
    EXPECT_EQ(PopCount(0x9102289248220842),16);
}
TEST(BitboardUtils, ResetLsb) {
    Bitboard b = 0x3102289248220840;
    EXPECT_EQ(ResetLsb(b), 6); //index returned
    EXPECT_EQ(b, (U64)0x3102289248220800); //LSB reset
    b = ZERO;
    EXPECT_EQ(b, (U64)0);
    b = ONE;
    EXPECT_EQ(b, (U64)1);
}
TEST(BitboardUtils, IsolateLsb) {
    Bitboard b = 127;
    EXPECT_EQ(IsolateLsb(b), (U64)1);
    b = 3377802801971200;
    EXPECT_EQ(IsolateLsb(b), (U64)131072);
}
TEST(BitboardUtils, RemoveLsb) {
    Bitboard b = 127;
    EXPECT_EQ(RemoveLsb(b), (U64)126);
    b = 3377802801971200;
    EXPECT_EQ(RemoveLsb(b), (U64)3377802801840128);
}
TEST(BitboardUtils, CreateMask) {
    U32 max = UINT32_MAX;
    int masked = 0;
    masked = max & CreateMask(0, 6);
    EXPECT_EQ(masked, 0x3f);
    masked = max & CreateMask(6, 12);
    EXPECT_EQ(masked, 0xfc0);
    masked = max & CreateMask(21, 23);
    EXPECT_EQ(masked, 0x600000);
}
