#pragma once

#include "Constants.h"

namespace Attacks {
    void Init();

    Bitboard AttacksPawns(COLOR color, int square);
    Bitboard AttacksKnights(int square);
    Bitboard AttacksKing(int square);
    Bitboard AttacksSliding(PIECE_TYPE pieceType, int square, Bitboard blockers);

    //Return the squares between two given squares. Strict straight/diagonal match is required (otherwise returns zero)
    Bitboard Between(int sq1, int sq2);
}
