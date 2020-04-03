#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>
#include <iostream>
#include <cassert>

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

const int INFINITE_SCORE = INFINITE_I16 - 1000;
const int MATESCORE = INFINITE_SCORE - 1000;

enum COLOR { WHITE, BLACK, NO_COLOR };
enum FILES { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH };
enum RANKS { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };
enum PIECE_TYPE { NO_PIECE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, ALL_PIECES };
enum PIECES { A_NOPIECE, W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING, W_ALLPIECES,
                        B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING, B_ALLPIECES, A_ALLPIECES };
enum DIRECTIONS { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };
const Bitboard ZERO = 0; //all zeros
const Bitboard ONE = 1;
const Bitboard ALL = (Bitboard)~0; //universal bitboard (all ones)
#define File(square) ((square) % 8)
#define Rank(square) ((square) / 8)

#define SquareBB(square) (ONE << (square))

#define PieceTypeToPieces(piecetype, color) (PIECES)(piecetype + 7 * color)
#define PiecesToPieceType(piece, color) (PIECE_TYPE)(piece - 7 * color)

#define IsValidPieceType(piece) (piece != NO_PIECE && piece != ALL_PIECES)
#define IsValidPiece(piece) (piece != A_NOPIECE && piece != W_ALLPIECES && piece != B_ALLPIECES && piece != A_ALLPIECES)

#define IsMateValue(score) (abs(score) <= MATESCORE && abs(score) > (MATESCORE - 1000))

//Output
#define P(x) std::cout << x << std::endl;
#define PL(x) std::cout << x << " " << std::flush;

//Debug
#ifdef DEBUG
    #define D(x) x
#else
    #define D(x) do {} while(0)
#endif

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

//Operators over enums
inline PIECE_TYPE& operator++(PIECE_TYPE& pieceType) {
    int i = static_cast<int>(pieceType);
    assert(i+1 > 0);
    assert(i+1 <= (int)ALL_PIECES);
    return pieceType = static_cast<PIECE_TYPE>(++i);
}

//Global inlined functions
inline int ColorlessRank(COLOR color, int square) {
    return color == WHITE ? Rank(square) : 7 ^ Rank(square);
}

#endif //CONSTANTS_H
