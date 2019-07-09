#ifndef _ATTACKS_H
#define _ATTACKS_H

#include "Constants.h"

namespace Attacks {

    void Init();

    Bitboard GetRay(DIRECTIONS direction, int square);

    Bitboard AttacksPawns(COLORS color, int square);
    Bitboard AttacksKnights(int square);
    Bitboard AttacksKing(int square);
    // Bitboard AttacksNonSliding(PIECE_TYPE pieceType, int square);
    Bitboard AttacksSliding(PIECE_TYPE pieceType, int square, Bitboard blockers);

    //Return the squares between two given squares
    Bitboard Between(int sq1, int sq2);

    //Helpers
    DIRECTIONS GetDirection(int sq1, int sq2);
    bool IsInDirection(PIECE_TYPE pieceType, int sq1, int sq2, DIRECTIONS &direction);
    bool IsInStraightDirection(int sq1, int sq2, DIRECTIONS &direction);
    bool IsInDiagonalDirection(int sq1, int sq2, DIRECTIONS &direction);
    DIRECTIONS OppositeDirection(DIRECTIONS direction);

    extern Bitboard m_BishopMask[64];
    extern Bitboard m_RookMask[64];
    extern Bitboard m_Rays[8][64]; //[DIRECTION][SQUARE]
    extern Bitboard m_NonSlidingAttacks[2][8][64]; //[COLOR][PIECE][SQUARE]

    extern Bitboard m_Between[64][64]; //[SQUARE][SQUARE]
}

#endif //_ATTACKS_H
