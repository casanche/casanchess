#pragma once

#include "Constants.h"

#include <bit>


namespace BitboardUtils {

    // -------------------------------
    // ----- Bitboard operations -----
    // -------------------------------

    // Return the index of the first '1' bit (Least Significant Bit, LSB)
    constexpr int BitscanForward(Bitboard b) {
        if(b == 0) return -1;
        return std::countr_zero(b);
    }

    // Return the index of the last '1' bit (Most Significant Bit, MSB)
    constexpr int BitscanReverse(Bitboard b) {
        if(b == 0) return -1;
        return 63 ^ std::countl_zero(b);
    }

    // Return a bitboard with the LSB only
    constexpr Bitboard IsolateLsb(Bitboard b) {
        return b & (~b + 1);
        // Note: x & -x
    }

    // Mirror the board in the north-south direction
    constexpr Bitboard Mirror(Bitboard bitboard) {
        return std::byteswap(bitboard);
    }

    // Return true if the bitboard has only one bit set
    constexpr bool OnlyOne(Bitboard b) {
        return std::has_single_bit(b);
    }

    // Count the number of set bits (population count)
    constexpr int PopCount(Bitboard b) {
        return std::popcount(b);
    }

    // Remove the LSB of the given bitboard
    constexpr void RemoveLsb(Bitboard &b) {
        b &= (b - 1);
        // Note: x & (x-1): removes the LSB (LSB reset)
    }

    // Reset the LSB of the given bitboard and return its index
    constexpr int ResetLsb(Bitboard &b) {
        int index = BitscanForward(b);
        RemoveLsb(b);

        return index;
    }

    // Move all bits to a given direction a certain number of times.
    // Bits falling off the edge are discarded
    constexpr Bitboard North(Bitboard bitboard, int times = 1) {
        return bitboard << 8*times;
    }
    constexpr Bitboard South(Bitboard bitboard, int times = 1) {
        return bitboard >> 8*times;
    }
    constexpr Bitboard West(Bitboard bitboard, int times = 1) {
        if(times < 0 || times >= 8) { return 0; } // Bits fall off the board
        return (bitboard >> times) & WestMask[times]; // Precalculated lookup table
    }
    constexpr Bitboard East(Bitboard bitboard, int times = 1) {
        if(times < 0 || times >= 8) { return 0; } // Bits fall off the board
        return (bitboard << times) & EastMask[times]; // Precalculated lookup table
    }

    // -----------------
    // ----- Masks -----
    // -----------------

    // BitMask(4, 2) create 4 bits as '1' (rest are '0') and move them 2 times
    // 0b11110000... -> 0b00111100...
    constexpr u64 BitMask(int width, int startIndex = 0) {
        return ((ONE << width) - 1) << startIndex;
    }
    // ClearMask(4, 2) create 4 bits as '0' (rest are '1') and move them 2 times
    // 0b00001111... -> 0b11000011...
    constexpr u64 ClearMask(int width, int startIndex = 0) {
        return ~BitMask(width, startIndex);
    }

    // --------------------------
    // ----- Bit operations -----
    // --------------------------

    // Give the value of the bit in index 'position' (0 = false, 1 = true)
    template <typename T>
    constexpr bool GetBit(T object, uint position) {
        if(position >= sizeof(T) * 8) {
            return false;
        }
        
        return (object >> position) & 1;
    }

    // First bits of the input number (first 16 bits in case of u16)
    // Typical use: Truncate Zobrist keys in Transposition Table.
    template<typename T>
    constexpr T UpperBits(u64 input) {
        constexpr int numBits = sizeof(T) * 8;
        constexpr int shift = 64 - numBits;
        return static_cast<T>(input >> shift);
    }

    // Take the first 'width' bits from the 'input' and place them from 'startIndex' in a u32 integer.
    // Example: PushBits(capturedType, 3, 18) take the 3 bits from 'capturedType' and place them in bits 18-20. All other bits are '0'.
    template <typename T>
    constexpr u32 PushBits(T input, int width, int startIndex) {
        return (input & BitMask(width)) << startIndex;
    }
    // Extract 'width' bits from the 'input' starting from 'startIndex' and return them displaced to the first position.
    // Example: RetrieveBits(m_move, 3, 18) take the 18-20 bits from 'm_move' and return them as a u32 integer. All other bits are '0'.
    template <typename T>
    constexpr u32 RetrieveBits(T input, int width, int startIndex) {
        // return (input & BitMask(width, startIndex)) >> startIndex;
        return (input >> startIndex) & BitMask(width);
    }

    // ---------------------------
    // ----- Defined in .cpp -----
    // ---------------------------

    void PrintBits(Bitboard bitboard);
} // namespace BitboardUtils

// -----------------------------
// ----- Bitboard Iterator -----
// -----------------------------
// Allows iterating over all the bits of a Bitboard
class BitboardIterator {
    Bitboard m_bitboard;

public:
    explicit constexpr BitboardIterator(Bitboard bitboard) : m_bitboard(bitboard) {}

    class Iterator {
        Bitboard b;
    public:
        explicit constexpr Iterator(Bitboard bitboard) : b(bitboard) {}

        constexpr int operator*() const {
            return BitboardUtils::BitscanForward(b);
        }
        constexpr Iterator& operator++() {
            BitboardUtils::RemoveLsb(b);
            return *this;
        }
        constexpr bool operator!=(const Iterator& other) const {
            return b != other.b;
        }
    };

    constexpr Iterator begin() const { return Iterator(m_bitboard); }
    constexpr Iterator end() const { return Iterator(0); }
};

using namespace BitboardUtils;
