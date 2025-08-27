#pragma once

#include "Constants.h"

#include <bit>

namespace BitboardUtils {
    //Gives the value of the bit in 'position'
    template <typename T>
    bool GetBit(T object, uint position) {
        if(position >= sizeof(T) * 8) {
            return false;
        }
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

    //First bits of the input number (16 if u16)
    template<typename T>
    constexpr T UpperBits(u64 input) {
        constexpr int numBits = sizeof(T) * 8;
        constexpr int shift = 64 - numBits;
        return static_cast<T>(input >> shift);
    }

    int BitscanForward(Bitboard b);
    int BitscanReverse(Bitboard b);
    int PopCount(Bitboard b);
    int ResetLsb(Bitboard &b);

    Bitboard IsolateLsb(Bitboard b);
    inline void RemoveLsb(Bitboard &b) { b &= (b - 1); };

    Bitboard North(Bitboard bitboard, int times = 1);
    Bitboard South(Bitboard bitboard, int times = 1);
    Bitboard West(Bitboard bitboard, int times = 1);
    Bitboard East(Bitboard bitboard, int times = 1);

    Bitboard Mirror(Bitboard bitboard);

    void PrintBits(Bitboard bitboard);
}

using namespace BitboardUtils;

class BitboardIterator {
public:
    explicit BitboardIterator(Bitboard bitboard) noexcept : m_bitboard(bitboard) {}

    class iterator {
        Bitboard b;
    public:
        explicit iterator(Bitboard bitboard) noexcept : b(bitboard) {}

        int operator*() const noexcept {
            return BitscanForward(b);
        }
        iterator& operator++() noexcept {
            RemoveLsb(b);
            return *this;
        }
        bool operator!=(const iterator& other) const noexcept {
            return b != other.b;
        }
    };

    iterator begin() const noexcept { return iterator(m_bitboard); }
    iterator end() const noexcept { return iterator(0); }
private:
    Bitboard m_bitboard;
};
