#include "BitboardUtils.h"

#include <iostream>

#ifdef _MSC_VER
    #include <intrin.h>
    #define __builtin_popcountll (int)__popcnt64
    #define __builtin_bswap64 _byteswap_uint64
#endif

/*
----------------
Basic operations
----------------
x & -x: get the LSB-only (LSB isolation)
x & (x-1): removes the LSB (LSB reset)
*/

//Gives the index of the first '1' bit (LSB)
int BitboardUtils::BitscanForward(Bitboard b) {
    #ifdef __GNUC__
        if (b)
            return __builtin_ffsll(b) - 1;
        return -1;
    #elif _MSC_VER
        unsigned long index;
        if (_BitScanForward64(&index, b))
            return index;
        else
            return -1;
    #endif
}

//Gives the index of the last '1' bit (MSB)
int BitboardUtils::BitscanReverse(Bitboard b) {
    #ifdef __GNUC__
        if(b)
            return 63 ^ __builtin_clzll(b);
        return -1;
    #elif _MSC_VER
        unsigned long index;
        if (_BitScanReverse64(&index, b))
            return index;
        else
            return -1;
    #endif
}

//Counts the number of set bits
int BitboardUtils::PopCount(Bitboard b) {
    return __builtin_popcountll(b);
}

//Resets the LSB of the given bitboard and returns its index
int BitboardUtils::ResetLsb(Bitboard &b) {
    int index = BitscanForward(b);
    b &= b - 1;
    return index;
}

//Returns a bitboard with the LSB only
Bitboard BitboardUtils::IsolateLsb(Bitboard b) {
    //return b & -b;
	return b & (~b + 1);
}

//Returns a bitboard removing the the LSB
Bitboard BitboardUtils::RemoveLsb(Bitboard b) {
    return b & (b - 1);
}

//Moves all bits to a given direction a certain number of times.
//All the bits falling off the edge are discarded
Bitboard BitboardUtils::North(Bitboard bitboard, int times) {
    return bitboard << 8*times;
}
Bitboard BitboardUtils::South(Bitboard bitboard, int times) {
    return bitboard >> 8*times;
}
Bitboard BitboardUtils::West(Bitboard bitboard, int times) {
    Bitboard newBitboard = bitboard;
    for(int i = 0; i < times; ++i) {
        newBitboard = (newBitboard >> 1) & ClearFile[FILEH];
    }
    return newBitboard;
}
Bitboard BitboardUtils::East(Bitboard bitboard, int times) {
    Bitboard newBitboard = bitboard;
    for(int i = 0; i < times; ++i) {
        newBitboard = (newBitboard << 1) & ClearFile[FILEA];
    }
    return newBitboard;
}

//Mirrors the board in the north-south direction
Bitboard BitboardUtils::Mirror(Bitboard bitboard) {
    return __builtin_bswap64(bitboard);
}

void BitboardUtils::PrintBits(Bitboard bitboard) {
    std::cout << "The bitboard: " << bitboard << ", ";
    std::cout << "Bit values: " << std::endl;
    int i = A8;
    while(i != -8) {
        std::cout << GetBit(bitboard, i) << " ";
        if(i % 8 == 7) {
            i -= 16;
            std::cout << std::endl;
        }
        i++;
    }
}

/*
const int BitTable[64] = {
  63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
  51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
  26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
  58, 20, 37, 17, 36, 8
};

int pop_1st_bit(uint64 *bb) {
  uint64 b = *bb ^ (*bb - 1);
  unsigned int fold = (unsigned) ((b & 0xffffffff) ^ (b >> 32));
  *bb &= (*bb - 1);
  return BitTable[(fold * 0x783a9b23) >> 26];
}

uint64 index_to_uint64(int index, int bits, uint64 m) {
  int i, j;
  uint64 result = 0ULL;
  for(i = 0; i < bits; i++) {
    j = pop_1st_bit(&m);
    if(index & (1 << i)) result |= (1ULL << j);
  }
  return result;
}
*/

U32 BitboardUtils::CreateMask(int startBit, int endBit) {
    int length = endBit - startBit;
    return ( (1 <<  length) - 1) << startBit;
}

//Other implementations
int BitboardUtils::BitscanForwardSimple(Bitboard bitboard) {
    int count;
    // bitboard ^= bitboard & (bitboard - 1);
    bitboard = (bitboard ^ (bitboard - 1) ) >> 1;
    for(count = 0; bitboard; ++count) {
        bitboard >>= 1;
    }
    return count;
}
