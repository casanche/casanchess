#include "Attacks.h"
#include "BitboardUtils.h"

Bitboard Attacks::m_Rays[8][64] = {{0}}; //[DIRECTION][SQUARE]
Bitboard Attacks::m_NonSlidingAttacks[2][8][64] = {{{0}}}; //[COLOR][PIECE][SQUARE]
Bitboard Attacks::m_Between[64][64] = {{0}}; //[SQUARE][SQUARE]

//Private functions
namespace {
    const u64 MIN_BIT = ONE;
    const u64 MAX_BIT = (ONE << 63);

    DIRECTIONS GetDirection(int sq1, int sq2) {
        if(sq1 == sq2)
            return NO_DIRECTION;

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

        return NO_DIRECTION;
    }

    Bitboard NextSquare(DIRECTIONS direction, Bitboard theSquare) {
        switch(direction) {
            case DIRECTIONS::NORTH: return North(theSquare); break;
            case DIRECTIONS::SOUTH: return South(theSquare); break;
            case DIRECTIONS::EAST:  return East(theSquare); break;
            case DIRECTIONS::WEST:  return West(theSquare); break;
            case DIRECTIONS::NORTH_EAST: return East(North(theSquare)); break;
            case DIRECTIONS::NORTH_WEST: return West(North(theSquare)); break;
            case DIRECTIONS::SOUTH_EAST: return East(South(theSquare)); break;
            case DIRECTIONS::SOUTH_WEST: return West(South(theSquare)); break;
            default: assert(false);
        }
        return ZERO;
    }

    Bitboard GenerateRay(DIRECTIONS direction, int square) {
        Bitboard ray = ZERO;
        Bitboard nextSquare = (ONE << square);

        do {
            nextSquare = NextSquare(direction, nextSquare);
            ray |= nextSquare;
        } while (nextSquare);

        return ray;
    }
}

void Attacks::Init() {
    //Rays
    for(int square = 0; square < 64; square++) {
        m_Rays[NORTH][square] = GenerateRay(NORTH, square);
        m_Rays[SOUTH][square] = GenerateRay(SOUTH, square);
        m_Rays[WEST][square] = GenerateRay(WEST, square);
        m_Rays[EAST][square] = GenerateRay(EAST, square);
        m_Rays[NORTH_WEST][square] = GenerateRay(NORTH_WEST, square);
        m_Rays[NORTH_EAST][square] = GenerateRay(NORTH_EAST, square);
        m_Rays[SOUTH_WEST][square] = GenerateRay(SOUTH_WEST, square);
        m_Rays[SOUTH_EAST][square] = GenerateRay(SOUTH_EAST, square);
    }

    //Pawn attacks
    for(int square = 0; square < 64; square++) {
        Bitboard thePawn = (ONE << square);

        //White pawns
        Bitboard attackLeft  = West(North(thePawn));
        Bitboard attackRight = East(North(thePawn));
        m_NonSlidingAttacks[WHITE][PAWN][square] = attackLeft | attackRight;

        //Black pawns
        attackLeft  = East(South(thePawn));
        attackRight = West(South(thePawn));
        m_NonSlidingAttacks[BLACK][PAWN][square] = attackLeft | attackRight;
    }

    //Knight attacks
    for(int square = 0; square < 64; square++) {
        Bitboard theKnight = (ONE << square);

        Bitboard pos1 = West(North(theKnight, 1), 2); //1 up, 2 left
        Bitboard pos2 = West(North(theKnight, 2), 1); //2 up, 1 left
        Bitboard pos3 = East(North(theKnight, 2), 1); //2 up, 1 right
        Bitboard pos4 = East(North(theKnight, 1), 2); //1 up, 2 right
        Bitboard pos5 = East(South(theKnight, 1), 2); //1 down, 2 right
        Bitboard pos6 = East(South(theKnight, 2), 1); //2 down, 1 right
        Bitboard pos7 = West(South(theKnight, 2), 1); //2 down, 1 left
        Bitboard pos8 = West(South(theKnight, 1), 2); //1 down, 2 left
        Bitboard attacks = (pos1 | pos2 | pos3 | pos4 | pos5 | pos6 | pos7 | pos8);

        m_NonSlidingAttacks[WHITE][KNIGHT][square] = attacks;
        m_NonSlidingAttacks[BLACK][KNIGHT][square] = attacks;
    }

    //King attacks
    for(int square = 0; square < 64; square++) {
        Bitboard theKing = (ONE << square);

        Bitboard right     = East(theKing);
        Bitboard left      = West(theKing);
        Bitboard up        = North(theKing);
        Bitboard down      = South(theKing);
        Bitboard upleft    = West(North(theKing));
        Bitboard upright   = East(North(theKing));
        Bitboard downleft  = West(South(theKing));
        Bitboard downright = East(South(theKing));
        Bitboard attacks = up | down | left | right | upleft | upright | downleft | downright;

        m_NonSlidingAttacks[WHITE][KING][square] = attacks;
        m_NonSlidingAttacks[BLACK][KING][square] = attacks;
    }

    //Between squares
    for(int sq1 = 0; sq1 < 64; sq1++) {
        for(int sq2 = 0; sq2 < 64; sq2++) {
            if(sq1 == sq2)
                continue;

            DIRECTIONS direction = GetDirection(sq1, sq2);
            Bitboard targetSquare = (ONE << sq2);

            bool straightOrDiagonal = m_Rays[direction][sq1] & targetSquare;
            if(!straightOrDiagonal)
                continue;

            m_Between[sq1][sq2] =  (m_Rays[direction][sq1] ^ m_Rays[direction][sq2]) & ~targetSquare; //ray from sq1 to sq2 (excluded)
        }
    }

}

Bitboard Attacks::GetRay(DIRECTIONS direction, int square) {
    return m_Rays[direction][square];
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

bool Attacks::IsInDirection(PIECE_TYPE pieceType, int sq1, int sq2) {
    bool inDirection = Between(sq1, sq2);

    if(inDirection) {
        DIRECTIONS direction = GetDirection(sq1, sq2);
        bool inStraightDirection = (direction == NORTH || direction == SOUTH || direction == EAST || direction == WEST);
        bool inDiagonalDirection = (direction == NORTH_EAST || direction == NORTH_WEST || direction == SOUTH_EAST || direction == SOUTH_WEST);

        if(pieceType == ROOK && inStraightDirection)
            return true;
        if(pieceType == BISHOP && inDiagonalDirection)
            return true;
    }
    return false;
}
