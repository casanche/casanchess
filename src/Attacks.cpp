#include "Attacks.h"
#include "BitboardUtils.h"

Bitboard Attacks::m_Rays[8][64] = {{0}}; //[DIRECTION][SQUARE]
Bitboard Attacks::m_NonSlidingAttacks[2][8][64] = {{{0}}}; //[COLOR][PIECE][SQUARE]
Bitboard Attacks::m_Between[64][64] = {{0}}; //[SQUARE][SQUARE]

//Private functions
namespace Attacks {
    const u64 MIN_BIT = ONE;
    const u64 MAX_BIT = (ONE << 63);

    DIRECTIONS GetDirection(int sq1, int sq2) {
        assert(sq1 != sq2);
        int deltaFile = File(sq2) - File(sq1);
        int deltaRank = Rank(sq2) - Rank(sq1);

        //East-West
        if(deltaRank == 0) {
            return deltaFile > 0 ? EAST : WEST;
        }
        //North-like
        else if(deltaRank > 0) {
            return deltaFile == 0 ? NORTH : 
                   deltaFile  > 0 ? NORTH_EAST : NORTH_WEST;
        }
        //South-like
        else if(deltaRank < 0) {
            return deltaFile == 0 ? SOUTH : 
                   deltaFile  > 0 ? SOUTH_EAST : SOUTH_WEST;
        }

        return NORTH;
    }

    int Shift(DIRECTIONS direction) {
        int shift = 0;
        switch(direction) {
            case DIRECTIONS::NORTH: shift = +8; break;
            case DIRECTIONS::SOUTH: shift = -8; break;
            case DIRECTIONS::EAST: shift = +1; break;
            case DIRECTIONS::WEST: shift = -1; break;
            case DIRECTIONS::NORTH_EAST: shift = +9; break;
            case DIRECTIONS::NORTH_WEST: shift = +7; break;
            case DIRECTIONS::SOUTH_EAST: shift = -7; break;
            case DIRECTIONS::SOUTH_WEST: shift = -9; break;
            default: assert(false);
        }
        return shift;
    }

    bool ContinueRayGeneration(DIRECTIONS direction, int square) {
        switch(direction) {
            case DIRECTIONS::NORTH: return Rank(square) < RANK8;
            case DIRECTIONS::SOUTH: return Rank(square) > RANK1;
            case DIRECTIONS::EAST: return File(square) < FILEH;
            case DIRECTIONS::WEST: return File(square) > FILEA;
            case DIRECTIONS::NORTH_EAST: return ContinueRayGeneration(NORTH, square) && ContinueRayGeneration(EAST, square);
            case DIRECTIONS::NORTH_WEST: return ContinueRayGeneration(NORTH, square) && ContinueRayGeneration(WEST, square);
            case DIRECTIONS::SOUTH_EAST: return ContinueRayGeneration(SOUTH, square) && ContinueRayGeneration(EAST, square);
            case DIRECTIONS::SOUTH_WEST: return ContinueRayGeneration(SOUTH, square) && ContinueRayGeneration(WEST, square);
            default: assert(false);
        }
        return 0;
    }
}

void Attacks::Init() {
    //Rays
    for(int square = 0; square < 64; square++) {
        m_Rays[NORTH][square] = GetRay(NORTH, square);
        m_Rays[SOUTH][square] = GetRay(SOUTH, square);
        m_Rays[WEST][square] = GetRay(WEST, square);
        m_Rays[EAST][square] = GetRay(EAST, square);
        m_Rays[NORTH_WEST][square] = GetRay(NORTH_WEST, square);
        m_Rays[NORTH_EAST][square] = GetRay(NORTH_EAST, square);
        m_Rays[SOUTH_WEST][square] = GetRay(SOUTH_WEST, square);
        m_Rays[SOUTH_EAST][square] = GetRay(SOUTH_EAST, square);
    }

    //Pawn attacks
    for(int square = 0; square < 64; square++) {
        Bitboard thePawn = ONE << square;

        //White pawns
        Bitboard attackLeft = (thePawn & ClearFile[FILEA]) << Shift(NORTH_WEST);
        Bitboard attackRight = (thePawn & ClearFile[FILEH]) << Shift(NORTH_EAST);

        m_NonSlidingAttacks[WHITE][PAWN][square] = attackLeft | attackRight;

        //Black pawns
        attackLeft = (thePawn & ClearFile[FILEH]) >> abs(Shift(SOUTH_EAST));
        attackRight = (thePawn & ClearFile[FILEA]) >> abs(Shift(SOUTH_WEST));

        m_NonSlidingAttacks[BLACK][PAWN][square] = attackLeft | attackRight;
    }

    //Knight attacks
    for(int square = 0; square < 64; square++) {
        Bitboard theKnight = (ONE << square);

        Bitboard pos1Clip = theKnight & ClearFile[FILEA] & ClearFile[FILEB];
        Bitboard pos2Clip = theKnight & ClearFile[FILEA];
        Bitboard pos3Clip = theKnight & ClearFile[FILEH];
        Bitboard pos4Clip = theKnight & ClearFile[FILEH] & ClearFile[FILEG];
        Bitboard pos5Clip = pos4Clip;
        Bitboard pos6Clip = pos3Clip;
        Bitboard pos7Clip = pos2Clip;
        Bitboard pos8Clip = pos1Clip;

        Bitboard pos1 = West(North(pos1Clip, 1), 2); //1 up, 2 left
        Bitboard pos2 = West(North(pos2Clip, 2), 1); //2 up, 1 left
        Bitboard pos3 = East(North(pos3Clip, 2), 1); //2 up, 1 right
        Bitboard pos4 = East(North(pos4Clip, 1), 2); //1 up, 2 right
        Bitboard pos5 = East(South(pos5Clip, 1), 2); //1 down, 2 right
        Bitboard pos6 = East(South(pos6Clip, 2), 1); //2 down, 1 right
        Bitboard pos7 = West(South(pos7Clip, 2), 1); //2 down, 1 left
        Bitboard pos8 = West(South(pos8Clip, 1), 2); //1 down, 2 left
        Bitboard attacks = (pos1 | pos2 | pos3 | pos4 | pos5 | pos6 | pos7 | pos8);

        m_NonSlidingAttacks[WHITE][KNIGHT][square] = attacks;
        m_NonSlidingAttacks[BLACK][KNIGHT][square] = attacks;
    }

    //King attacks
    for(int square = 0; square < 64; square++) {
        Bitboard theKing = ONE << square;

        Bitboard leftClip = theKing & ClearFile[FILEA];
        Bitboard rightClip = theKing & ClearFile[FILEH];

        Bitboard right = East(rightClip);
        Bitboard left = West(leftClip);
        Bitboard up = North(theKing);
        Bitboard down = South(theKing);
        Bitboard upleft = leftClip << Shift(NORTH_WEST);
        Bitboard upright = rightClip << Shift(NORTH_EAST);
        Bitboard downleft = leftClip >> abs(Shift(SOUTH_WEST));
        Bitboard downright = rightClip >> abs(Shift(SOUTH_EAST));
        Bitboard attacks = up | down | left | right | upleft | upright | downleft | downright;

        m_NonSlidingAttacks[WHITE][KING][square] = attacks;
        m_NonSlidingAttacks[BLACK][KING][square] = attacks;
    }

    //Between squares
    for(int sq1 = 0; sq1 < 64; sq1++) {
        for(int sq2 = 0; sq2 < 64; sq2++) {
            if(sq1 == sq2) continue;

            DIRECTIONS direction = GetDirection(sq1, sq2);
            int shift = Shift(direction);

            if( (sq2-sq1) % shift ) { //not a straight or diagonal line
                m_Between[sq1][sq2] = ZERO;
            }
            else for(int i = sq1 + shift; i != sq2; i += shift) {
                m_Between[sq1][sq2] |= (ONE << i);
            }
        }
    }

}

Bitboard Attacks::GetRay(DIRECTIONS direction, int square) {
    Bitboard ray = ZERO;
    int cursor = square;
    int shift = Shift(direction);

    while(ContinueRayGeneration(direction, cursor)) {
        cursor += shift;
        ray |= (ONE << cursor);
    }
    return ray;
}

Bitboard Attacks::AttacksPawns(COLOR color, int square) {
    return m_NonSlidingAttacks[color][PAWN][square];
}
Bitboard Attacks::AttacksKnights(int square) {
    return m_NonSlidingAttacks[WHITE][KNIGHT][square];
}
Bitboard Attacks::AttacksKing(int square) {
    return m_NonSlidingAttacks[WHITE][KING][square];
}
//Classical approach
Bitboard Attacks::AttacksSliding(PIECE_TYPE pieceType, int square, Bitboard blockers) {
    Bitboard attacks = ZERO;

    switch(pieceType) {
        case BISHOP: {
            Bitboard nw = m_Rays[NORTH_WEST][square];
            Bitboard ne = m_Rays[NORTH_EAST][square];
            Bitboard sw = m_Rays[SOUTH_WEST][square];
            Bitboard se = m_Rays[SOUTH_EAST][square];

            Bitboard nwBlockers = blockers & nw;
            Bitboard neBlockers = blockers & ne;
            Bitboard swBlockers = blockers & sw;
            Bitboard seBlockers = blockers & se;

            attacks |= nw ^ m_Rays[NORTH_WEST][ BitscanForward(nwBlockers | MAX_BIT) ];
            attacks |= ne ^ m_Rays[NORTH_EAST][ BitscanForward(neBlockers | MAX_BIT) ];
            attacks |= sw ^ m_Rays[SOUTH_WEST][ BitscanReverse(swBlockers | MIN_BIT) ];
            attacks |= se ^ m_Rays[SOUTH_EAST][ BitscanReverse(seBlockers | MIN_BIT) ];

        }
            break;

        case ROOK: {
            Bitboard north = m_Rays[NORTH][square];
            Bitboard east = m_Rays[EAST][square];
            Bitboard south = m_Rays[SOUTH][square];
            Bitboard west = m_Rays[WEST][square];

            Bitboard northBlockers = blockers & north;
            Bitboard eastBlockers = blockers & east;
            Bitboard southBlockers = blockers & south;
            Bitboard westBlockers = blockers & west;

            attacks |= north ^ m_Rays[NORTH][ BitscanForward(northBlockers | MAX_BIT) ];
            attacks |= east ^ m_Rays[EAST][ BitscanForward(eastBlockers | MAX_BIT) ];
            attacks |= south ^ m_Rays[SOUTH][ BitscanReverse(southBlockers | MIN_BIT) ];
            attacks |= west ^ m_Rays[WEST][ BitscanReverse(westBlockers | MIN_BIT) ];
        }
            break;
            
        case QUEEN: {
            attacks |= AttacksSliding(BISHOP, square, blockers);
            attacks |= AttacksSliding(ROOK, square, blockers);
        }
            break;

        default: assert(false);
    };

    return attacks;
}

Bitboard Attacks::Between(int sq1, int sq2) {
    return m_Between[sq1][sq2];
}

bool Attacks::IsInDirection(PIECE_TYPE pieceType, int sq1, int sq2, DIRECTIONS &direction) {
    bool inDirection = Between(sq1, sq2);

    if(inDirection) {
        direction = GetDirection(sq1, sq2);
        bool inStraightDirection = (direction == NORTH || direction == SOUTH || direction == EAST || direction == WEST);
        bool inDiagonalDirection = (direction == NORTH_EAST || direction == NORTH_WEST || direction == SOUTH_EAST || direction == SOUTH_WEST);

        if(pieceType == ROOK && inStraightDirection)
            return true;
        if(pieceType == BISHOP && inDiagonalDirection)
            return true;
    }
    return false;
}
