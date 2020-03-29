#ifndef ZOBRIST_KEYS_H
#define ZOBRIST_KEYS_H

#include "Constants.h"
#include "Move.h"
#include <random>

class Board;

namespace ZobristKeys {
    void Init();
    
    extern u64 m_zkeyPieces[2][8][64];
    extern u64 m_zkeyCastling[2][2];
    extern u64 m_zkeyEnpassant[8];
    extern u64 m_zkeyColor;
}

class ZobristKey {
public:
    ZobristKey();
    inline u64 Key() const { return m_key; };
    void SetKey(Board& board);
    inline void SetKey(u64 key) { m_key = key; }
    void SetPawnKey(Board& board);
    void UpdateColor();
    void UpdateEnpassant(Bitboard enpassant);
    void UpdatePiece(COLOR color, PIECE_TYPE pieceType, int square);
    void UpdateCastling(CASTLING_TYPE castlingType);
private:
    u64 m_key;
};

class PRNG {
public:
    PRNG(); //seed
    u64 Random();
private:
    std::random_device m_device;
    std::mt19937_64 m_mersenne;
    std::uniform_int_distribution<u64> m_distribution;
};

#endif //ZOBRIST_KEYS_H
