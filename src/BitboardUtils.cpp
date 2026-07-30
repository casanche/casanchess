#include "BitboardUtils.h"

#include <iostream>


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
