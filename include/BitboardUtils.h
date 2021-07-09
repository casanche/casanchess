#ifndef BITBOARDUTILS_H
#define BITBOARDUTILS_H

#include "Constants.h"

namespace BitboardUtils {
    //Gives the value of the bit in 'position'
    template <typename T>
    bool GetBit(T object, int position) {
        return (object >> position) & 1;
    }

    //BitMask(4) returns 0b1111
    constexpr u64 BitMask(int width, int startIndex = 0) {
        return ((ONE << width) - 1) << startIndex;
    }
    constexpr u64 ClearMask(int width, int startIndex = 0) {
        return ~BitMask(width, startIndex);
    }

    //Take 'width' bits from the 'input', and move them 'startIndex' times
    template <typename T>
    inline u32 PushBits(T input, int width, int startIndex) {
        return (input & BitMask(width)) << startIndex;
    }
    //Take 'width' bits from the 'input', starting from 'startIndex'
    template <typename T>
    inline u32 RetrieveBits(T input, int width, int startIndex) {
        return (input & BitMask(width, startIndex)) >> startIndex;
    }

    int BitscanForward(Bitboard b);
    int BitscanReverse(Bitboard b);
    int PopCount(Bitboard b);
    int ResetLsb(Bitboard &b);

    Bitboard IsolateLsb(Bitboard b);
    void RemoveLsb(Bitboard &b);

    Bitboard North(Bitboard bitboard, int times = 1);
    Bitboard South(Bitboard bitboard, int times = 1);
    Bitboard West(Bitboard bitboard, int times = 1);
    Bitboard East(Bitboard bitboard, int times = 1);

    Bitboard Mirror(Bitboard bitboard);

    void PrintBits(Bitboard bitboard);
}

using namespace BitboardUtils;

#endif //BITBOARDUTILS_H