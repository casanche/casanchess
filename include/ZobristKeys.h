#ifndef _ZOBRIST_KEYS_H
#define _ZOBRIST_KEYS_H

#include "Constants.h"
#include "Move.h"
#include <random>

class Board;

namespace ZobristKeys {
    void Init();
    
    extern U64 m_zkeyPieces[2][8][64];
    extern U64 m_zkeyCastling[2][2];
    extern U64 m_zkeyEnpassant[8];
    extern U64 m_zkeyColor;
}

class ZobristKey {
public:
    ZobristKey();
    inline U64 Key() const { return m_key; };
    void SetKey(Board& board);
    inline void SetKey(U64 key) { m_key = key; }
    void UpdateColor();
    void UpdateEnpassant(Bitboard enpassant);
    void UpdatePiece(COLORS color, PIECE_TYPE pieceType, int square);
    void UpdateCastling(CASTLING_TYPE castlingType);
private:
    U64 m_key;
};

class PRNG {
public:
    PRNG(); //seed
    U64 Random();
private:
    std::random_device m_device;
    std::mt19937_64 m_mersenne;
    std::uniform_int_distribution<U64> m_distribution;
};

#endif //_ZOBRIST_KEYS_H
