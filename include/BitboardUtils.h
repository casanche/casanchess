#ifndef BITBOARDUTILS_H
#define BITBOARDUTILS_H

#include "Constants.h"

namespace BitboardUtils {
    //Gives the value of the bit in 'position'
    template <typename T>
    bool GetBit(const T& object, int position) {
        return (object >> position) & 1;
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