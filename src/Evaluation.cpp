#include "Evaluation.h"
using namespace Evaluation;

#include "Attacks.h"
#include "Board.h"
#include "BitboardUtils.h"

//Constants
const Bitboard LIGHT_SQUARES = 0x55AA55AA55AA55AA;
const Bitboard DARK_SQUARES = 0xAA55AA55AA55AA55;

//Game phase
//16 is the maximum, we enter endgame with 8
const int MAX_PHASEMATERIAL = 16;
const int PHASE_WEIGHT[8] = {
    0, //NO_PIECE
    0, //PAWN
    1, //KNIGHT
    1, //BISHOP
    1, //ROOK
    2, //QUEEN
    0 //KING
};

const int KS_BONUS_PIECETYPE[2][8] = {
    {0, 0, 40, 40, 60, 100, 0},
    {0, 0, 10, 10, 20, 30, 0}
}; //[INNER/OUTER][PIECE_TYPE]

//Precomputed tables
Bitboard Evaluation::ADJACENT_FILES[8] = {0};
Bitboard Evaluation::ADJACENT_RANKS[8] = {0};
Bitboard Evaluation::PASSED_PAWN_FRONT[2][64] = {{0}};
Bitboard Evaluation::PASSED_PAWN_SIDES[2][64] = {{0}};
Bitboard Evaluation::PASSED_PAWN_AREA[2][64] = {{0}};
Bitboard Evaluation::KING_INNER_RING[64] = {0};
Bitboard Evaluation::KING_OUTER_RING[64] = {0};
Bitboard Evaluation::KING_SAFETY_TABLE[128] = {0};

class Evaluation::Score {
public:
    Score() {}
    void Add(int score) {
        m_taperedScore.mg += score;
        m_taperedScore.eg += score;
    }
    void Add(int mg, int eg) {
        m_taperedScore.mg += mg;
        m_taperedScore.eg += eg;
    }
    void Add(TaperedScore score) {
        m_taperedScore.mg += score.mg;
        m_taperedScore.eg += score.eg;
    }
    void Subtract(int mg, int eg) {
        m_taperedScore.mg -= mg;
        m_taperedScore.eg -= eg;
    }
    void Subtract(TaperedScore score) {
        m_taperedScore.mg -= score.mg;
        m_taperedScore.eg -= score.eg;
    }
    int Tapered(int phase) {
        return TaperedCalculation(m_taperedScore.mg, m_taperedScore.eg, phase);
    }
private:
    int TaperedCalculation(int mgScore, int egScore, int phase);

    TaperedScore m_taperedScore;
};

int Evaluation::Score::TaperedCalculation(int mgScore, int egScore, int phase) {
    assert(phase <= MAX_PHASEMATERIAL);

    int score = mgScore * phase + egScore * (MAX_PHASEMATERIAL - phase);
    score /= 2 * MAX_PHASEMATERIAL;
    return score;
}

void Evaluation::Init() {
    //Adjacent files
    for(int file = FILEA; file <= FILEH; file++) {
        if(file != FILEA) ADJACENT_FILES[file] |= MaskFile[file-1];
        if(file != FILEH) ADJACENT_FILES[file] |= MaskFile[file+1];
    }
    //Adjacent ranks
    for(int rank = RANK1; rank <= RANK8; rank++) {
        if(rank != RANK1) ADJACENT_RANKS[rank] |= MaskRank[rank-1];
        if(rank != RANK8) ADJACENT_RANKS[rank] |= MaskRank[rank+1];
    }
    //Passed pawn area
    for(int square = 0; square < 64; square++) {
        PASSED_PAWN_FRONT[WHITE][square] = Attacks::GetRay(NORTH, square);
        PASSED_PAWN_FRONT[BLACK][square] = Attacks::GetRay(SOUTH, square);
        if(Rank(square) == Rank(square+1)) {
            PASSED_PAWN_SIDES[WHITE][square] |= Attacks::GetRay(NORTH, square+1);
            PASSED_PAWN_SIDES[BLACK][square] |= Attacks::GetRay(SOUTH, square+1);
        }
        if(Rank(square) == Rank(square-1)) {
            PASSED_PAWN_SIDES[WHITE][square] |= Attacks::GetRay(NORTH, square-1);
            PASSED_PAWN_SIDES[BLACK][square] |= Attacks::GetRay(SOUTH, square-1);
        }
        PASSED_PAWN_AREA[WHITE][square] = PASSED_PAWN_FRONT[WHITE][square] | PASSED_PAWN_SIDES[WHITE][square];
        PASSED_PAWN_AREA[BLACK][square] = PASSED_PAWN_FRONT[BLACK][square] | PASSED_PAWN_SIDES[BLACK][square];
    }
    //King inner and outer rings
    for(int square = 0; square < 64; square++) {
        KING_INNER_RING[square] = Attacks::AttacksKing(square);
        //Outer ring is the sum of the attacks from each square of the inner ring, minus the inner ring and the king square
        Bitboard bb = KING_INNER_RING[square];
        while(bb) {
            int innerSquare = ResetLsb(bb);
            KING_OUTER_RING[square] |= Attacks::AttacksKing(innerSquare);
        }
        KING_OUTER_RING[square] ^= KING_INNER_RING[square] | SquareBB(square);
    }
    //King safety
    for(int i = 0; i < 128; i++) {
        KING_SAFETY_TABLE[i] = Evaluation::Sigmoid(i, KS_MAXBONUS, KS_MIDPOINT, KS_SLOPE);
    }
}

bool Evaluation::AreHeavyPieces(const Board& board) {
    COLORS color = board.ActivePlayer();
    return board.Piece(color, ALL_PIECES) ^ (board.Piece(color, PAWN) | board.Piece(color, KING));
}

bool Evaluation::InsufficientMaterial(const Board &board) {
    return !board.Piece(WHITE, PAWN) && !board.Piece(BLACK, PAWN)  //no pawns
        && PopCount( board.AllPieces() ) == 3  //one heavy piece (ignoring kings)...
        && PopCount( board.Piece(WHITE, KNIGHT) | board.Piece(WHITE, BISHOP) | board.Piece(BLACK, KNIGHT) | board.Piece(BLACK, BISHOP) );  //...knight or bishop
}

bool Evaluation::IsSemiopenFile(const Board& board, COLORS color, int square) {
    return !( board.Piece(color, PAWN) & MaskFile[ File(square) ] );
}

void Evaluation::PawnAttacks(const Board& board, Bitboard attacksMobility[2][8]) {
    Bitboard thePawns = board.Piece(WHITE, PAWN);
    attacksMobility[WHITE][PAWN] = (thePawns & ClearFile[FILEA]) << 7 | (thePawns & ClearFile[FILEH]) << 9;
    thePawns = board.Piece(BLACK, PAWN);
    attacksMobility[BLACK][PAWN] = (thePawns & ClearFile[FILEH]) >> 7 | (thePawns & ClearFile[FILEA]) >> 9;
}

int Evaluation::Phase(const Board& board) {
    return PHASE_WEIGHT[KNIGHT] * PopCount(board.Piece(WHITE, KNIGHT))
         + PHASE_WEIGHT[BISHOP] * PopCount(board.Piece(WHITE, BISHOP))
         + PHASE_WEIGHT[ROOK] * PopCount(board.Piece(WHITE, ROOK))
         + PHASE_WEIGHT[QUEEN] * PopCount(board.Piece(WHITE, QUEEN))

         + PHASE_WEIGHT[KNIGHT] * PopCount(board.Piece(BLACK, KNIGHT))
         + PHASE_WEIGHT[BISHOP] * PopCount(board.Piece(BLACK, BISHOP))
         + PHASE_WEIGHT[ROOK] * PopCount(board.Piece(BLACK, ROOK))
         + PHASE_WEIGHT[QUEEN] * PopCount(board.Piece(BLACK, QUEEN));
}

TaperedScore Evaluation::EvalBishopPair(const Board &board, COLORS color) {
    TaperedScore score;
    if( (board.Piece(color, BISHOP) & LIGHT_SQUARES) && (board.Piece(color, BISHOP) & DARK_SQUARES) ) {
        score.mg = params.BISHOP_PAIR[MG];
        score.eg = params.BISHOP_PAIR[EG];
    }
    return score;
}

void Evaluation::EvalKingSafety(const Board &board, Bitboard attacksMobility[2][8], Score& score) {
    int kingSafetyUnits[2] = {0}; //[COLOR]

    for(COLORS color : {WHITE, BLACK}) {
        Bitboard enemyKing = board.Piece((COLORS)!color, KING);
        int kingSquare = BitscanForward(enemyKing);
        Bitboard kingInnerRing = KING_INNER_RING[kingSquare];
        Bitboard kingOuterRing = KING_OUTER_RING[kingSquare];

        Bitboard enemyPawnAttacks = attacksMobility[(COLORS)!color][PAWN];
        Bitboard pawnRestrictions = board.Piece(color, PAWN) | enemyPawnAttacks;

        Bitboard enemyAttacks = attacksMobility[(COLORS)!color][ALL_PIECES];

        for(PIECE_TYPE pieceType = KNIGHT; pieceType <= QUEEN; ++pieceType) {
            kingSafetyUnits[color] += PopCount(attacksMobility[color][pieceType] & kingInnerRing & ~pawnRestrictions & ~enemyAttacks) * KS_BONUS_PIECETYPE[0][pieceType] * 2;
            kingSafetyUnits[color] += PopCount(attacksMobility[color][pieceType] & kingInnerRing & ~pawnRestrictions & enemyAttacks) * KS_BONUS_PIECETYPE[0][pieceType];
            kingSafetyUnits[color] += PopCount(attacksMobility[color][pieceType] & kingOuterRing & ~pawnRestrictions) * KS_BONUS_PIECETYPE[1][pieceType];
        }
    }

    kingSafetyUnits[WHITE] /= 10;
    kingSafetyUnits[BLACK] /= 10;

    if(kingSafetyUnits[WHITE] > 127) kingSafetyUnits[WHITE] = 127;
    if(kingSafetyUnits[BLACK] > 127) kingSafetyUnits[BLACK] = 127;

    score.Add     ( KING_SAFETY_TABLE[ kingSafetyUnits[WHITE] ], 0 );
    score.Subtract( KING_SAFETY_TABLE[ kingSafetyUnits[BLACK] ], 0 );
}

void Evaluation::EvalMaterial(const Board& board, Score& score) {
    for(PIECE_TYPE pieceType : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
        int countBalance = PopCount( board.Piece(WHITE,pieceType) ) - PopCount( board.Piece(BLACK,pieceType) );
        score.Add(
            countBalance * params.MATERIAL_VALUES[MG][pieceType],
            countBalance * params.MATERIAL_VALUES[EG][pieceType]
        );
    }
}

#ifdef DEBUG_PAWN_HASH
int test_total = 0;
int test_hit = 0;
int test_miss = 0;
#endif

//Try to retrieve the pawn structure from hash. If not found, perform the calculation
TaperedScore Evaluation::EvalPawns(const Board &board) {
    #ifdef DEBUG_PAWN_HASH
    if(test_total % 1000 == 0) {
        P(board.PawnKey());
        P("PawnHash hitrate: " << test_total << ", " << test_hit << ", " << test_miss \
                               << ", " << (float)test_hit / test_total * 100 << "%");
    }
    test_total++;
    #endif

    PawnEntry* pawnEntry = Hash::pawnHash.ProbeEntry( board.PawnKey() );
    if(pawnEntry) {
        #ifdef DEBUG_PAWN_HASH
        test_hit++;
        #endif

        return TaperedScore( pawnEntry->evalMg,
                             pawnEntry->evalEg );
    }
    else {
        //Calculate
        #ifdef DEBUG_PAWN_HASH
        test_miss++;
        #endif

        TaperedScore whiteEval = EvalPawnsCalculation(board, WHITE);
        TaperedScore blackEval = EvalPawnsCalculation(board, BLACK);
        TaperedScore score;
        score.mg = whiteEval.mg - blackEval.mg;
        score.eg = whiteEval.eg - blackEval.eg;

        //Store in hash
        Hash::pawnHash.AddEntry(board.PawnKey(), score.mg, score.eg);

        return score;
    }
}

TaperedScore Evaluation::EvalPawnsCalculation(const Board &board, COLORS color) {
    TaperedScore score;

    Bitboard thePawns = board.Piece(color, PAWN);
    Bitboard enemyPawns = board.Piece((COLORS)!color, PAWN);

    //--Double pawns
    //1-rank distance
    int doubledPawns = PopCount(thePawns & North(thePawns));
    score.mg += doubledPawns * params.DOUBLED_PAWN[MG];
    score.eg += doubledPawns * params.DOUBLED_PAWN[EG];
    //2-rank distance: half bonus
    doubledPawns = PopCount(thePawns & North(thePawns, 2));
    score.mg += doubledPawns * params.DOUBLED_PAWN[MG] / 2;
    score.eg += doubledPawns * params.DOUBLED_PAWN[EG] / 2;

    Bitboard bb = thePawns;
    while(bb) {
        int square = ResetLsb(bb);

        //Psqt
        int index = SQUARE_CONVERSION[color][square];
        score.mg += PSQT[PAWN][index];
        score.eg += PSQT_ENDGAME[PAWN][index];

        int file = File(square);
        // Bitboard pushSquare = color == WHITE ? North(SquareBB(square)) : South(SquareBB(square));
        bool isPassed = !(PASSED_PAWN_AREA[color][square] & enemyPawns);
        // bool isDoubled = thePawns & PASSED_PAWN_FRONT[color][square]; //instead of pushSquare
        bool isIsolated = !(thePawns & ADJACENT_FILES[file]);
        // bool isBackward = enemyPawnsAttacks & pushSquare;
        // bool isBackward = thePawns & ADJACENT_FILES[file];

        if(isPassed) {
            int rank = ColorlessRank(color, square);
            // bool isFreePassed = !(PASSED_PAWN_FRONT[color][square] & board.Piece((COLORS)!color, ALL_PIECES));
            // if(isFreePassed) {
            //     const bool isUnstoppable = false;
            //     if(isUnstoppable) {
            //         score.mg += 800;
            //         score.eg += 800;
            //     } else {
            //         score.mg += 2 * PASSED_PAWN[MG][rank];
            //         score.eg += 2 * PASSED_PAWN[EG][rank];
            //     }
            // } else {
                score.mg += params.PASSED_PAWN[MG][rank];
                score.eg += params.PASSED_PAWN[EG][rank];
            // }
        }
        if(isIsolated) {
            int rank = ColorlessRank(color, square);
            score.mg += params.ISOLATED_PAWN[MG][rank];
            score.eg += params.ISOLATED_PAWN[EG][rank];
        }
    }

    return score;
}

TaperedScore Evaluation::EvalRookOpen(const Board& board, COLORS color) {
    TaperedScore score;
    Bitboard bb = board.Piece(color, ROOK);
    while(bb) {
        int square = ResetLsb(bb);
        //Semi-open files
        if( IsSemiopenFile(board, color, square) ) {
            score.mg += params.ROOK_SEMIOPEN[MG];
            score.eg += params.ROOK_SEMIOPEN[EG];

            COLORS enemyColor = (COLORS)!color;
            Bitboard theKing = board.Piece(enemyColor, KING);
            int file = File(square);
            bool inKingFile = theKing & MaskFile[file];
            bool inAdjacentKingFile = theKing & ADJACENT_FILES[file];
            if(inKingFile) {
                score.mg += params.KING_SEMIOPEN[MG];
                score.eg += params.KING_SEMIOPEN[EG];
            }
            else if(inAdjacentKingFile) {
                score.mg += params.KING_SEMIOPEN_ADJACENT[MG];
                score.eg += params.KING_SEMIOPEN_ADJACENT[EG];
            }
            //Open Files
            if( IsSemiopenFile(board, enemyColor, square) ) {
                score.mg += params.ROOK_OPEN[MG];
                score.eg += params.ROOK_OPEN[EG];
                if(inKingFile) {
                    score.mg += params.KING_OPEN[MG];
                    score.eg += params.KING_OPEN[EG];
                }
                else if(inAdjacentKingFile) {
                    score.mg += params.KING_OPEN_ADJACENT[MG];
                    score.eg += params.KING_OPEN_ADJACENT[EG];
                }
            }
        }
    }
    return score;
}

int Evaluation::Evaluate(const Board& board) {
    Score score;
    Bitboard attacksMobility[2][8] = {{0}}; //[COLOR][PIECE_TYPE]

    //Automatic draws
    if( InsufficientMaterial(board) )
        return 0;
        
    //Material
    EvalMaterial(board, score);

    PawnAttacks(board, attacksMobility);

    //Psqt and mobility
    for(COLORS color : {WHITE, BLACK}) {
        const int sign = color == WHITE ? 1 : -1;

        //Pawn restrictions for mobility and king safety
        Bitboard ownPawns = board.Piece(color, PAWN);
        Bitboard enemyPawnAttacks = attacksMobility[(COLORS)!color][PAWN];
        Bitboard pawnRestrictions = ownPawns | enemyPawnAttacks;

        for(PIECE_TYPE pieceType = KNIGHT; pieceType <= KING; ++pieceType) {
              
            Bitboard bb = board.Piece(color, pieceType);
            while(bb) {
                int square = ResetLsb(bb);

                //Psqt
                int index = SQUARE_CONVERSION[color][square];
                score.Add(
                    sign * PSQT[pieceType][index],
                    sign * PSQT_ENDGAME[pieceType][index]
                );

                //Mobility
                if(pieceType == KNIGHT) {
                    Bitboard attacks = Attacks::AttacksKnights(square);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * params.MOBILITY_KNIGHT[MG][mob],
                        sign * params.MOBILITY_KNIGHT[EG][mob]
                    );
                }
                else if(pieceType == BISHOP) {
                    Bitboard blockers = board.AllPieces() ^ (board.Piece(color, BISHOP) | board.Piece(color, QUEEN));
                    Bitboard attacks = Attacks::AttacksSliding(BISHOP, square, blockers);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * params.MOBILITY_BISHOP[MG][mob],
                        sign * params.MOBILITY_BISHOP[EG][mob]
                    );
                }
                else if(pieceType == ROOK) {
                    Bitboard blockers = board.AllPieces() ^ (board.Piece(color, ROOK) | board.Piece(color, QUEEN));
                    Bitboard attacks = Attacks::AttacksSliding(ROOK, square, blockers);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * params.MOBILITY_ROOK[MG][mob],
                        sign * params.MOBILITY_ROOK[EG][mob]
                    );
                }
                else if(pieceType == QUEEN) {
                    //Bishop-like movement
                    Bitboard blockers = board.AllPieces() ^ (board.Piece(color, BISHOP) | board.Piece(color, QUEEN));
                    Bitboard diagonal = Attacks::AttacksSliding(BISHOP, square, blockers);
                    //Rook-like movement
                    blockers = board.AllPieces() ^ (board.Piece(color, ROOK) | board.Piece(color, QUEEN));
                    Bitboard straight = Attacks::AttacksSliding(ROOK, square, blockers);

                    Bitboard attacks = diagonal | straight;
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;

                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * params.MOBILITY_QUEEN[MG][mob],
                        sign * params.MOBILITY_QUEEN[EG][mob]
                    );
                }
            }
        } //pieceType
    } //color

    EvalKingSafety(board, attacksMobility, score);

    //Pawns
    score.Add( EvalPawns(board) );

    //Rook open files
    score.Add     ( EvalRookOpen(board, WHITE) );
    score.Subtract( EvalRookOpen(board, BLACK) );
    
    //Bishop pair
    score.Add     ( EvalBishopPair(board, WHITE) );
    score.Subtract( EvalBishopPair(board, BLACK) );

    const int sign = board.ActivePlayer() == WHITE ? 1 : -1;
    return sign * score.Tapered( Phase(board) );
}
