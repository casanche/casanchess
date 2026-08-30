#pragma once

#include "Constants.h"
#include "Move.h"

class Board;

namespace ZobristKeys {
    void Init();
    
    extern u64 m_zkeyColor;
    extern u64 m_zkeyPieces[2][8][64]; //[COLOR][PIECE_TYPE][SQUARE]
    extern u64 m_zkeyCastling[2][2]; //[COLOR][CASTLING_TYPE]
    extern u64 m_zkeyEnpassant[8]; //[FILE]
}

class ZobristKey {
public:
    ZobristKey();

    inline u64 Key() const { return m_key; };

    void SetKey(Board& board);
    inline void SetKey(u64 key) { m_key = key; }
    void SetPawnKey(Board& board);

    void UpdateColor();
    void UpdatePiece(COLOR color, PIECE_TYPE pieceType, int square);
    void UpdateCastling(CASTLING_TYPE castlingType);
    void UpdateEnpassant(Bitboard enpassant);
private:
    u64 m_key;
};
