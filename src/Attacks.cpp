#include "Attacks.h"

#include "BitboardUtils.h"

Bitboard Attacks::m_BishopMask[64] = {0};
Bitboard Attacks::m_RookMask[64] = {0};
Bitboard Attacks::m_Rays[8][64] = {{0}};
Bitboard Attacks::m_NonSlidingAttacks[2][8][64] = {{{0}}};
Bitboard Attacks::m_Between[64][64] = {{0}};

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
    
    //Bishop mask
    for(int square = 0; square < 64; square++) {
        m_BishopMask[square] = AttacksSliding(BISHOP, square, ZERO);
    }

    //Rook mask
    for(int square = 0; square < 64; square++) {
        m_RookMask[square] = AttacksSliding(ROOK, square, ZERO);
    }

    //Pawn attacks
    for(int square = 0; square < 64; square++) {
        Bitboard thePawn = ONE << square;

        //White pawns
        Bitboard attackLeft = (thePawn & ClearFile[FILEA]) << 7;
        Bitboard attackRight = (thePawn & ClearFile[FILEH]) << 9;

        m_NonSlidingAttacks[WHITE][PAWN][square] = attackLeft | attackRight;

        //Black pawns
        attackLeft = (thePawn & ClearFile[FILEH]) >> 7;
        attackRight = (thePawn & ClearFile[FILEA]) >> 9;

        m_NonSlidingAttacks[BLACK][PAWN][square] = attackLeft | attackRight;
    }

    //Knight attacks
    for(int square = 0; square < 64; square++) {
        Bitboard theKnight = ONE << square;

        Bitboard pos1Clip = theKnight & ClearFile[FILEA] & ClearFile[FILEB];
        Bitboard pos2Clip = theKnight & ClearFile[FILEA];
        Bitboard pos3Clip = theKnight & ClearFile[FILEH];
        Bitboard pos4Clip = theKnight & ClearFile[FILEH] & ClearFile[FILEG];
        Bitboard pos5Clip = pos4Clip;
        Bitboard pos6Clip = pos3Clip;
        Bitboard pos7Clip = pos2Clip;
        Bitboard pos8Clip = pos1Clip;

        Bitboard pos1 = pos1Clip << 8 >> 2; //1 up, 2 left
        Bitboard pos2 = pos2Clip << 8*2 >> 1; //2 up, 1 left
        Bitboard pos3 = pos3Clip << 8*2 << 1; //2 up, 1 right
        Bitboard pos4 = pos4Clip << 8 << 2; //1 up, 2 right
        Bitboard pos5 = pos5Clip >> 8 << 2; //1 down, 2 right
        Bitboard pos6 = pos6Clip >> 8*2 << 1; //2 down, 1 right
        Bitboard pos7 = pos7Clip >> 8*2 >> 1; //2 down, 1 left
        Bitboard pos8 = pos8Clip >> 8 >> 2; //1 down, 2 left
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
        Bitboard upleft = leftClip << 8 >> 1;
        Bitboard upright = rightClip << 8 << 1;
        Bitboard downleft = leftClip >> 8 >> 1;
        Bitboard downright = rightClip >> 8 << 1;
        Bitboard attacks = up | down | left | right | upleft | upright | downleft | downright;

        m_NonSlidingAttacks[WHITE][KING][square] = attacks;
        m_NonSlidingAttacks[BLACK][KING][square] = attacks;
    }

    //Between squares
    for(int sq1 = 0; sq1 < 64; sq1++) {
        for(int sq2 = 0; sq2 < 64; sq2++) {

            if(sq1 == sq2) continue;

            DIRECTIONS direction = GetDirection(sq1, sq2);
            switch(direction) {
                case NORTH: {
                    const int nextSq = 8;
                    for(int i = sq1 + nextSq; i != sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case SOUTH: {
                    const int nextSq = -8;
                    for(int i = sq1 + nextSq; i != sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case EAST: {
                    const int nextSq = 1;
                    for(int i = sq1 + nextSq; i != sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case WEST: {
                    const int nextSq = -1;                    
                    for(int i = sq1 + nextSq; i != sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case NORTH_EAST: {
                    const int nextSq = 9;
                    if( (sq2-sq1) % nextSq ) m_Between[sq1][sq2] = ZERO;
                    else for(int i = sq1 + nextSq; i < sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case NORTH_WEST: {
                    const int nextSq = 7;
                    if( (sq2-sq1) % nextSq ) m_Between[sq1][sq2] = ZERO;
                    else for(int i = sq1 + nextSq; i < sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case SOUTH_EAST: {
                    const int nextSq = -7;
                    if( (sq2-sq1) % nextSq ) m_Between[sq1][sq2] = ZERO;
                    else for(int i = sq1 + nextSq; i > sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                case SOUTH_WEST: {
                    const int nextSq = -9;
                    if( (sq2-sq1) % nextSq ) m_Between[sq1][sq2] = ZERO;
                    else for(int i = sq1 + nextSq; i > sq2; i += nextSq) { m_Between[sq1][sq2] |= (ONE << i); }
                } break;
                default: assert(false);
            };
        }
    }

}

Bitboard Attacks::GetRay(DIRECTIONS direction, int square) {
    switch(direction) {
    case DIRECTIONS::NORTH:
        return 0x0101010101010100ULL << square;
    case DIRECTIONS::SOUTH:
        return 0x0080808080808080ULL >> (63 - square);
    case DIRECTIONS::WEST:
        return (ONE << square) - (ONE << (square & 56));
    case DIRECTIONS::EAST:
        return 2 * ((ONE << (square | 7)) - (ONE << square));
    case DIRECTIONS::NORTH_WEST:
        return West(0x102040810204000ULL, 7 - File[square] ) << (Rank(square) * 8);
    case DIRECTIONS::NORTH_EAST:
        return East(0x8040201008040200ULL, File[square] ) << (Rank(square) * 8);
    case DIRECTIONS::SOUTH_WEST:
        return West(0x40201008040201ULL, 7 - File[square] ) >> ( (7 - Rank(square)) * 8);
    case DIRECTIONS::SOUTH_EAST:
        return East(0x2040810204080ULL, File[square] ) >> ( (7 - Rank(square)) * 8);
    default:
        break;
    }
    return 0;
}

//Classical approach
Bitboard Attacks::AttacksPawns(COLORS color, int square) {
    return m_NonSlidingAttacks[color][PAWN][square];
}
Bitboard Attacks::AttacksKnights(int square) {
    return m_NonSlidingAttacks[WHITE][KNIGHT][square];
}
Bitboard Attacks::AttacksKing(int square) {
    return m_NonSlidingAttacks[WHITE][KING][square];
}
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

            // attacks |= nw & ~m_Rays[NORTH_WEST][BitscanForward(nwBlockers)];
            // attacks |= ne & ~m_Rays[NORTH_EAST][BitscanForward(neBlockers)];
            // attacks |= sw & ~m_Rays[SOUTH_WEST][BitscanForward(swBlockers)];
            // attacks |= se & ~m_Rays[SOUTH_EAST][BitscanForward(seBlockers)];

            attacks |= (nwBlockers) ? nw & ~m_Rays[NORTH_WEST][BitscanForward(nwBlockers)] : nw;
            attacks |= (neBlockers) ? ne & ~m_Rays[NORTH_EAST][BitscanForward(neBlockers)] : ne;
            attacks |= (swBlockers) ? sw & ~m_Rays[SOUTH_WEST][BitscanReverse(swBlockers)] : sw;
            attacks |= (seBlockers) ? se & ~m_Rays[SOUTH_EAST][BitscanReverse(seBlockers)] : se;

        }
            break;

        case ROOK: {
            Bitboard north = m_Rays[NORTH][square];
            Bitboard south = m_Rays[SOUTH][square];
            Bitboard west = m_Rays[WEST][square];
            Bitboard east = m_Rays[EAST][square];

            Bitboard northBlockers = blockers & north;
            Bitboard southBlockers = blockers & south;
            Bitboard westBlockers = blockers & west;
            Bitboard eastBlockers = blockers & east;

            // attacks |= north & ~m_Rays[NORTH][BitscanForward(northBlockers)];
            // attacks |= south & ~m_Rays[SOUTH][BitscanForward(southBlockers)];
            // attacks |= west & ~m_Rays[WEST][BitscanForward(westBlockers)];
            // attacks |= east & ~m_Rays[EAST][BitscanForward(eastBlockers)];

            attacks |= (northBlockers) ? north & ~m_Rays[NORTH][BitscanForward(northBlockers)] : north;
            attacks |= (southBlockers) ? south & ~m_Rays[SOUTH][BitscanReverse(southBlockers)] : south;
            attacks |= (westBlockers) ? west & ~m_Rays[WEST][BitscanReverse(westBlockers)] : west;
            attacks |= (eastBlockers) ? east & ~m_Rays[EAST][BitscanForward(eastBlockers)] : east;
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

//The direction between square1 and square2. No strict straight/diagonal match is required
DIRECTIONS Attacks::GetDirection(int sq1, int sq2) {
    int deltaSquare = sq2 - sq1;
    int deltaFile = File(sq2) - File(sq1);
    int deltaRank = Rank(sq2) - Rank(sq1);

    assert(sq1 != sq2);

    if(deltaFile == 0) {
        if(deltaSquare > 0) return NORTH;
        if(deltaSquare < 0) return SOUTH;
    }
    if(deltaRank == 0) {
        if(deltaSquare > 0) return EAST;
        if(deltaSquare < 0) return WEST;
    }
    if(deltaRank > 0) { //North-like
        if(deltaFile > 0) return NORTH_EAST;
        if(deltaFile < 0) return NORTH_WEST;
    } else if(deltaRank < 0) { //South-like
        if(deltaFile > 0) return SOUTH_EAST;
        if(deltaFile < 0) return SOUTH_WEST;
    }

    assert(false);
    return NORTH;
}

bool Attacks::IsInDirection(PIECE_TYPE pieceType, int sq1, int sq2, DIRECTIONS &direction) {
    if(pieceType == ROOK) {
        return IsInStraightDirection(sq1, sq2, direction);
    } else if(pieceType == BISHOP) {
        return IsInDiagonalDirection(sq1, sq2, direction);
    }
    assert(false);
    return false;
}
bool Attacks::IsInStraightDirection(int sq1, int sq2, DIRECTIONS &direction) {
    int deltaSquare = sq2 - sq1;
    int deltaFile = File(sq2) - File(sq1);
    int deltaRank = Rank(sq2) - Rank(sq1);

    assert(sq1 != sq2);

    if(deltaFile == 0) {
        if(deltaSquare > 0) { direction = NORTH; return true; }
        if(deltaSquare < 0) { direction = SOUTH; return true; }
    }
    if(deltaRank == 0) {
        if(deltaSquare > 0) { direction = EAST; return true; }
        if(deltaSquare < 0) { direction = WEST; return true; }
    }
    return false;
}
bool Attacks::IsInDiagonalDirection(int sq1, int sq2, DIRECTIONS &direction) {
    int deltaFile = File(sq2) - File(sq1);
    int deltaRank = Rank(sq2) - Rank(sq1);

    assert(sq1 != sq2);

    //North-like
    if(deltaRank > 0 && abs(deltaFile) == abs(deltaRank) ) {
        if(deltaFile > 0) { direction = NORTH_EAST; return true; }
        if(deltaFile < 0) { direction = NORTH_WEST; return true; }
    }
    //South-like
    else if(deltaRank < 0 && abs(deltaFile) == abs(deltaRank) ) {
        if(deltaFile > 0) { direction = SOUTH_EAST; return true; }
        if(deltaFile < 0) { direction = SOUTH_WEST; return true; }
    }
    return false;
}
DIRECTIONS Attacks::OppositeDirection(DIRECTIONS direction) {
    switch(direction) {
        case NORTH: return SOUTH; break;
        case SOUTH: return NORTH; break;
        case EAST: return WEST; break;
        case WEST: return EAST; break;
        case NORTH_EAST: return SOUTH_WEST; break;
        case NORTH_WEST: return SOUTH_EAST; break;
        case SOUTH_EAST: return NORTH_WEST; break;
        case SOUTH_WEST: return NORTH_EAST; break;
    };
    assert(false);
    return NORTH;
}