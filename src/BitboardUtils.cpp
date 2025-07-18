#include "BitboardUtils.h"

#include <bit>
#include <iostream>

/*
----------------
Basic operations
----------------
x & -x: get the LSB-only (LSB isolation)
x & (x-1): removes the LSB (LSB reset)
*/

// Returns the index of the first '1' bit (LSB)
int BitboardUtils::BitscanForward(Bitboard b) {
    if(b)
        return std::countr_zero(b);
    return -1;
}

//Gives the index of the last '1' bit (MSB)
int BitboardUtils::BitscanReverse(Bitboard b) {
    if(b)
        return 63 ^ std::countl_zero(b);
    return -1;
}

//Counts the number of set bits
int BitboardUtils::PopCount(Bitboard b) {
    return std::popcount(b);
}

//Resets the LSB of the given bitboard and returns its index
int BitboardUtils::ResetLsb(Bitboard &b) {
    int index = BitscanForward(b);
    RemoveLsb(b);
    return index;
}

//Returns a bitboard with the LSB only
Bitboard BitboardUtils::IsolateLsb(Bitboard b) {
	return b & (~b + 1);
}

void BitboardUtils::RemoveLsb(Bitboard &b) {
    b &= (b - 1);
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
    // Optimized: use precalculated lookup table
    if (times >= 0 && times < 8) {
        return (bitboard >> times) & WestMask[times];
    }
    // For larger values, return 0 as bits fall off the board
    return 0;
}
Bitboard BitboardUtils::East(Bitboard bitboard, int times) {
    // Optimized: use precalculated lookup table
    if (times >= 0 && times < 8) {
        return (bitboard << times) & EastMask[times];
    }
    // For larger values, return 0 as bits fall off the board
    return 0;
}

//Mirrors the board in the north-south direction
Bitboard BitboardUtils::Mirror(Bitboard bitboard) {
    return std::byteswap(bitboard);
}

void BitboardUtils::PrintBits(Bitboard bitboard) {
    std::cout << "The bitboard: " << bitboard << ", ";
    std::cout << "Bit values: " << std::endl;
    int square = A8;
    const int nextRank = 8;
    while(square >= 0) {
        std::cout << GetBit(bitboard, square) << " ";
        if(File(square) == FILEH) {
            square -= nextRank * 2; //go down two ranks
            std::cout << std::endl;
        }
        square++;
    }
}
