#include "BitboardUtils.h"
#include "Utils.h"

#include <gtest/gtest.h>

//https://github.com/google/googletest/blob/master/googletest/docs/primer.md
//ASSERT_* generate fatal failures, EXPECT_* generate nonfatal failures (preferred)
//EXPECT_TRUE, EXPECT_FALSE
//EXPECT_EQ, EXPECT_NE, EXPECT_LT, EXPECT_LE, EXPECT_GT, EXPECT_GE
//EXPECT_STREQ, EXPECT_STRNE, EXPECT_STRCASEEQ, EXPECT_STRCASENE

//Utils
TEST(Utils, RandomNumberGenerator_64bits) {
    uint64_t r1, r2;

    //Fixed seed
    Utils::PRNG_64 prng_fixedSeed_1(100);
    Utils::PRNG_64 prng_fixedSeed_2(100);
    r1 = prng_fixedSeed_1.Random();
    r2 = prng_fixedSeed_2.Random();
    EXPECT_EQ(r1, r2);

    //Random seed
    Utils::PRNG_64 prng_noSeed_1;
    Utils::PRNG_64 prng_noSeed_2;
    r1 = prng_noSeed_1.Random();
    r2 = prng_noSeed_2.Random();
    EXPECT_NE(r1, r2);

    Utils::PRNG_64 prng_noSeed_explicit_1(0);
    Utils::PRNG_64 prng_noSeed_explicit_2(0);
    r1 = prng_noSeed_explicit_1.Random();
    r2 = prng_noSeed_explicit_2.Random();
    EXPECT_NE(r1, r2);
}

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
    EXPECT_EQ(b, (u64)0x3102289248220800); //LSB reset
    b = ZERO;
    EXPECT_EQ(b, (u64)0);
    b = ONE;
    EXPECT_EQ(b, (u64)1);
}
TEST(BitboardUtils, IsolateLsb) {
    Bitboard b = 127;
    EXPECT_EQ(IsolateLsb(b), (u64)1);
    b = 3377802801971200;
    EXPECT_EQ(IsolateLsb(b), (u64)131072);
}
TEST(BitboardUtils, RemoveLsb) {
    Bitboard b = 127;
    RemoveLsb(b);
    EXPECT_EQ(b, (u64)126);
    
    b = 3377802801971200;
    RemoveLsb(b);
    EXPECT_EQ(b, (u64)3377802801840128);
}
