#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cassert>
#include <cstdint>
#include <iostream>

typedef unsigned int uint;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t Bitboard;
#define INFINITE __INT_MAX__
#define INFINITE_U64 __UINT64_MAX__
#define INFINITE_I16 __INT16_MAX__

#ifdef _MSC_VER
    #define __INT_MAX__ INT_MAX
    #define __UINT64_MAX__ ULLONG_MAX
    #define __INT16_MAX__ SHRT_MAX
#endif

const int INFINITE_SCORE = INFINITE_I16 - 1024;
const int MATESCORE = INFINITE_SCORE - 1024;

const int MAX_PLY = 2560;
const int SEE_MATERIAL_VALUES[8] = {0, 100, 350, 350, 500, 1050, 0}; //[PIECE]

enum COLOR { WHITE, BLACK, NO_COLOR };
enum FILES { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH };
enum RANKS { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };
enum PIECE_TYPE { NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES };
enum DIRECTIONS { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST, NO_DIRECTION };

const Bitboard ZERO = 0; //all zeros
const Bitboard ONE = 1;
const Bitboard ALL = (Bitboard)~0; //universal bitboard (all ones)
#define File(square) ((square) % 8)
#define Rank(square) ((square) / 8)

#define SquareBB(square) (ONE << (square))

//Debug
#ifdef DEBUG
    #define D(x) x
#else
    #define D(x) ((void)0)
#endif

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
const u8 File[64] = {
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH,
    FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH
};

const Bitboard MaskRank[8] = {
    (u64)0xff,
    (u64)MaskRank[0] << 8*1,
    (u64)MaskRank[0] << 8*2,
    (u64)MaskRank[0] << 8*3,
    (u64)MaskRank[0] << 8*4,
    (u64)MaskRank[0] << 8*5,
    (u64)MaskRank[0] << 8*6,
    (u64)MaskRank[0] << 8*7
};
const Bitboard MaskFile[8] = {
    (u64)0x101010101010101,
    (u64)MaskFile[0] << 1,
    (u64)MaskFile[0] << 2,
    (u64)MaskFile[0] << 3,
    (u64)MaskFile[0] << 4,
    (u64)MaskFile[0] << 5,
    (u64)MaskFile[0] << 6,
    (u64)MaskFile[0] << 7
};
const Bitboard ClearRank[8] = {
    ~MaskRank[RANK1],
    ~MaskRank[RANK2],
    ~MaskRank[RANK3],
    ~MaskRank[RANK4],
    ~MaskRank[RANK5],
    ~MaskRank[RANK6],
    ~MaskRank[RANK7],
    ~MaskRank[RANK8]
};
const Bitboard ClearFile[8] = {
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
const Bitboard WestMask[8] = {
    ALL,                                    // 0 moves
    ClearFile[FILEH],                       // 1 move
    ClearFile[FILEH] & ClearFile[FILEG],    // 2 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF], // 3 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE], // 4 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED], // 5 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED] & ClearFile[FILEC], // 6 moves
    ClearFile[FILEH] & ClearFile[FILEG] & ClearFile[FILEF] & ClearFile[FILEE] & ClearFile[FILED] & ClearFile[FILEC] & ClearFile[FILEB] // 7 moves
};

const Bitboard EastMask[8] = {
    ALL,                                    // 0 moves
    ClearFile[FILEA],                       // 1 move
    ClearFile[FILEA] & ClearFile[FILEB],    // 2 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC], // 3 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED], // 4 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE], // 5 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE] & ClearFile[FILEF], // 6 moves
    ClearFile[FILEA] & ClearFile[FILEB] & ClearFile[FILEC] & ClearFile[FILED] & ClearFile[FILEE] & ClearFile[FILEF] & ClearFile[FILEG] // 7 moves
};

//Operators over enums
inline PIECE_TYPE& operator++(PIECE_TYPE& pieceType) {
    int i = static_cast<int>(pieceType);
    assert(i+1 > 0);
    assert(i+1 <= (int)ALL_PIECES);
    return pieceType = static_cast<PIECE_TYPE>(++i);
}

//Global inlined functions
inline bool IsValidPieceType(PIECE_TYPE piece) {
    return (piece != NO_PIECE) && (piece != ALL_PIECES);
}
inline bool IsMateValue(int score) {
    return (abs(score) <= MATESCORE) && (abs(score) > (MATESCORE - 1024));
}
inline int RelativeRank(COLOR color, int square) {
    return color == WHITE ? Rank(square) : 7 ^ Rank(square);
}
inline Bitboard RelativeMaskRank(COLOR color, RANKS rank) {
    return color == WHITE ? MaskRank[rank] : MaskRank[RANK8-rank];
}

#endif //CONSTANTS_H
