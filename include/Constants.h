#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

using uint = unsigned int;
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using Bitboard = uint64_t;

constexpr int INFINITE = std::numeric_limits<int>::max();
constexpr u64 INFINITE_U64 = std::numeric_limits<u64>::max();
constexpr i16 INFINITE_I16 = std::numeric_limits<i16>::max();

constexpr i16 INFINITE_SCORE = INFINITE_I16 - 1024;
constexpr i16 MATESCORE_MAX = INFINITE_SCORE - 1024;
constexpr i16 MATESCORE_MIN = MATESCORE_MAX - 1024;
constexpr i16 TBWIN = MATESCORE_MIN - 1;
constexpr i16 WINSCORE = TBWIN - 1024;

constexpr i16 NO_SCORE = -INFINITE_I16;
constexpr i16 NO_EVAL = -INFINITE_I16;

constexpr int MAX_DEPTH = 128;
constexpr int MAX_PLY = 256;
constexpr int MAX_PLY_HISTORY = 2048;

namespace SEE{
    constexpr int MATERIAL_VALUES[8] = {0, 100, 350, 350, 500, 1050, 20000}; // [PIECE]
}

enum COLOR { WHITE, BLACK, NO_COLOR };
enum FILES { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH };
enum RANKS { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };
enum PIECE_TYPE { NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES };
enum DIRECTIONS { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST, NO_DIRECTION };

constexpr Bitboard ZERO = 0; //all zeros
constexpr Bitboard ONE = 1;
constexpr Bitboard ALL = (Bitboard)~0; //universal bitboard (all ones)

constexpr int File(int square) {
    assert(square >= 0 && square < 64);
    return square & 7; //square % 8
}
constexpr int Rank(int square) {
    assert(square >= 0 && square < 64);
    return square >> 3; //square / 8
}

constexpr Bitboard SquareBB(int square) {
    assert(square >= 0 && square < 64);
    return ONE << square;
}

#define P(x) std::cout << x << std::endl;
#define PL(x) std::cout << x << " " << std::flush;

enum SQUARES {
    A1=0,  B1, C1, D1, E1, F1, G1, H1,
    A2=8,  B2, C2, D2, E2, F2, G2, H2,
    A3=16, B3, C3, D3, E3, F3, G3, H3,
    A4=24, B4, C4, D4, E4, F4, G4, H4,
    A5=32, B5, C5, D5, E5, F5, G5, H5,
    A6=40, B6, C6, D6, E6, F6, G6, H6,
    A7=48, B7, C7, D7, E7, F7, G7, H7,
    A8=56, B8, C8, D8, E8, F8, G8, H8
};

constexpr Bitboard MaskRank[8] = {
    (u64)0xff,
    (u64)MaskRank[0] << 8*1,
    (u64)MaskRank[0] << 8*2,
    (u64)MaskRank[0] << 8*3,
    (u64)MaskRank[0] << 8*4,
    (u64)MaskRank[0] << 8*5,
    (u64)MaskRank[0] << 8*6,
    (u64)MaskRank[0] << 8*7
};
constexpr Bitboard MaskFile[8] = {
    (u64)0x101010101010101,
    (u64)MaskFile[0] << 1,
    (u64)MaskFile[0] << 2,
    (u64)MaskFile[0] << 3,
    (u64)MaskFile[0] << 4,
    (u64)MaskFile[0] << 5,
    (u64)MaskFile[0] << 6,
    (u64)MaskFile[0] << 7
};
constexpr Bitboard ClearRank[8] = {
    ~MaskRank[RANK1],
    ~MaskRank[RANK2],
    ~MaskRank[RANK3],
    ~MaskRank[RANK4],
    ~MaskRank[RANK5],
    ~MaskRank[RANK6],
    ~MaskRank[RANK7],
    ~MaskRank[RANK8]
};
constexpr Bitboard ClearFile[8] = {
    ~MaskFile[FILEA],
    ~MaskFile[FILEB],
    ~MaskFile[FILEC],
    ~MaskFile[FILED],
    ~MaskFile[FILEE],
    ~MaskFile[FILEF],
    ~MaskFile[FILEG],
    ~MaskFile[FILEH],
};

// Precalculated masks for West/East operations (optimization)
// Each entry contains the combined mask for that many moves
constexpr Bitboard WestMask[8] = {
    ALL,                                    // 0 moves
    ClearFile[FILEH],                       // 1 move
    ClearFile[FILEH] & ClearFile[FILEG],    // 2 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF], // 3 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE], // 4 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED], // 5 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED] & ClearFile[FILEC], // 6 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED] & ClearFile[FILEC] & ClearFile[FILEB] // 7 moves
};

constexpr Bitboard EastMask[8] = {
    ALL,                                    // 0 moves
    ClearFile[FILEA],                       // 1 move
    ClearFile[FILEA] & ClearFile[FILEB],    // 2 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC], // 3 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED], // 4 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE], // 5 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE] & ClearFile[FILEF], // 6 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE] & ClearFile[FILEF] & ClearFile[FILEG] // 7 moves
};

//Logarithm lookup table
constexpr int LOG_TABLE_SIZE = 512;
constexpr int LOG_TABLE_SCALE = 100; // Scale factor to integer conversion
extern const std::array<u16, LOG_TABLE_SIZE> LogTable;

//Casts
constexpr int CastInt(double value) {
    return static_cast<int>(value);
}
constexpr u8 SafeCastU8(int value) {
    assert(value >= 0 && value <= std::numeric_limits<u8>::max());
    return static_cast<u8>(value);
}
constexpr i16 SafeCastInt16(int value) {
    assert(value >= std::numeric_limits<i16>::min() && value <= std::numeric_limits<i16>::max());
    return static_cast<i16>(value);
}

//Operators over enums
constexpr PIECE_TYPE& operator++(PIECE_TYPE& pieceType) {
    int i = CastInt(pieceType);
    assert(i+1 > 0);
    assert(i+1 <= (int)ALL_PIECES);
    return pieceType = static_cast<PIECE_TYPE>(++i);
}

//Global inlined functions
constexpr bool IsValidPieceType(PIECE_TYPE piece) {
    return (piece != NO_PIECE) && (piece != ALL_PIECES);
}
constexpr bool IsMateValue(const int score) {
    return (abs(score) >= MATESCORE_MIN) && (abs(score) <= MATESCORE_MAX);
}
constexpr bool IsWinValue(const int score) {
    return abs(score) >= WINSCORE;
}
constexpr int RelativeRank(COLOR color, int square) {
    return color == WHITE ? Rank(square) : 7 ^ Rank(square);
}
constexpr Bitboard RelativeMaskRank(COLOR color, RANKS rank) {
    return color == WHITE ? MaskRank[rank] : MaskRank[RANK8-rank];
}
