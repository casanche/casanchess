#ifndef ATTACKS_H
#define ATTACKS_H

#include "Constants.h"

namespace Attacks {
    void Init();

    Bitboard GetRay(DIRECTIONS direction, int square);

    Bitboard AttacksPawns(COLOR color, int square);
    Bitboard AttacksKnights(int square);
    Bitboard AttacksKing(int square);
    Bitboard AttacksSliding(PIECE_TYPE pieceType, int square, Bitboard blockers);

    //Return the squares between two given squares. Strict straight/diagonal match is required (otherwise returns zero)
    Bitboard Between(int sq1, int sq2);

    //Helpers
    bool IsInDirection(PIECE_TYPE pieceType, int sq1, int sq2);

    //Lookup tables
    extern Bitboard m_Rays[8][64]; //[DIRECTION][SQUARE]
    extern Bitboard m_NonSlidingAttacks[2][8][64]; //[COLOR][PIECE][SQUARE]
    extern Bitboard m_Between[64][64]; //[SQUARE][SQUARE]
}

#endif //ATTACKS_H
