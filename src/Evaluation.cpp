#include "Evaluation.h"
using namespace Evaluation;

#include "Attacks.h"
#include "Board.h"
#include "BitboardUtils.h"
#include "NNUE.h"

namespace Evaluation {

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

    //Debug
    int test_total = 0;
    int test_hit = 0;
    int test_miss = 0;

    //S-shaped function that starts at zero and grows until the maximum
    //f(x) = g(x) - g(0), where g(x) = a / (1 + exp(-c * (x-b)) )
    double Sigmoid(double x, double maximum, double midPoint, double slope) {
        return maximum / (1 + exp( -slope * (x - midPoint) ) )
            - maximum / (1 + exp( -slope * (0 - midPoint) ) );
    }

    //Precomputed tables
    Bitboard ADJACENT_FILES[8] = {0};
    Bitboard ADJACENT_RANKS[8] = {0};
    Bitboard PASSED_PAWN_FRONT[2][64] = {{0}};
    Bitboard PASSED_PAWN_SIDES[2][64] = {{0}};
    Bitboard PASSED_PAWN_AREA[2][64] = {{0}};
    Bitboard KING_INNER_RING[64] = {0};
    Bitboard KING_OUTER_RING[64] = {0};
    u16 KING_SAFETY_TABLE[128] = {0};

    const u8 SQUARE_CONVERSION[2][64] = { //[COLOR][SQUARE]
        //WHITE
        {
            56, 57, 58, 59, 60, 61, 62, 63,
            48, 49, 50, 51, 52, 53, 54, 55,
            40, 41, 42, 43, 44, 45, 46, 47,
            32, 33, 34, 35, 36, 37, 38, 39,
            24, 25, 26, 27, 28, 29, 30, 31,
            16, 17, 18, 19, 20, 21, 22, 23,
            8,   9, 10, 11, 12, 13, 14, 15,
            0,   1,  2,  3,  4,  5,  6,  7
        },
        //BLACK
        {
            7,   6,  5,  4,  3,  2,  1,  0,
            15, 14, 13, 12, 11, 10,  9,  8,
            23, 22, 21, 20, 19, 18, 17, 16,
            31, 30, 29, 28, 27, 26, 25, 24,
            39, 38, 37, 36, 35, 34, 33, 32,
            47, 46, 45, 44, 43, 42, 41, 40,
            55, 54, 53, 52, 51, 50, 49, 48,
            63, 62, 61, 60, 59, 58, 57, 56
        }
    };

} //namespace Evaluation

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
    int score = mgScore * phase + egScore * (MAX_PHASEMATERIAL - phase);
    score /= MAX_PHASEMATERIAL;
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
    //King safety
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
    for(int i = 0; i < 128; i++) {
        KING_SAFETY_TABLE[i] = static_cast<u16>(
            Evaluation::Sigmoid(i, parameters.KS_SIGMOID[0], parameters.KS_SIGMOID[1], parameters.KS_SIGMOID[2] / 1000.f)
        );
    }
}

bool Evaluation::AreHeavyPieces(const Board& board) {
    COLOR color = board.ActivePlayer();
    return board.Piece(color, ALL_PIECES) ^ (board.Piece(color, PAWN) | board.Piece(color, KING));
}

bool Evaluation::InsufficientMaterial(const Board &board) {
    return !board.Piece(WHITE, PAWN) && !board.Piece(BLACK, PAWN)  //no pawns
        && PopCount( board.AllPieces() ) == 3  //one heavy piece (ignoring kings)...
        && PopCount( board.Piece(WHITE, KNIGHT) | board.Piece(WHITE, BISHOP) | board.Piece(BLACK, KNIGHT) | board.Piece(BLACK, BISHOP) );  //...knight or bishop
}

bool Evaluation::IsSemiopenFile(const Board& board, COLOR color, int square) {
    return !( board.Piece(color, PAWN) & MaskFile[ File(square) ] );
}

void Evaluation::PawnAttacks(const Board& board, Bitboard attacksMobility[2][8]) {
    Bitboard thePawns = board.Piece(WHITE, PAWN);
    attacksMobility[WHITE][PAWN] = (thePawns & ClearFile[FILEA]) << 7 | (thePawns & ClearFile[FILEH]) << 9;
    thePawns = board.Piece(BLACK, PAWN);
    attacksMobility[BLACK][PAWN] = (thePawns & ClearFile[FILEH]) >> 7 | (thePawns & ClearFile[FILEA]) >> 9;
}

int Evaluation::Phase(const Board& board) {
    return PHASE_WEIGHT[KNIGHT] * PopCount(board.Piece(WHITE, KNIGHT) | board.Piece(BLACK, KNIGHT))
         + PHASE_WEIGHT[BISHOP] * PopCount(board.Piece(WHITE, BISHOP) | board.Piece(BLACK, BISHOP))
         + PHASE_WEIGHT[ROOK] * PopCount(board.Piece(WHITE, ROOK) | board.Piece(BLACK, ROOK))
         + PHASE_WEIGHT[QUEEN] * PopCount(board.Piece(WHITE, QUEEN) | board.Piece(BLACK, QUEEN));
}

TaperedScore Evaluation::EvalBishopPair(const Board &board, COLOR color) {
    TaperedScore score;
    if( (board.Piece(color, BISHOP) & LIGHT_SQUARES) && (board.Piece(color, BISHOP) & DARK_SQUARES) ) {
        score.mg = parameters.BISHOP_PAIR[MG];
        score.eg = parameters.BISHOP_PAIR[EG];
    }
    return score;
}

void Evaluation::EvalKingSafety(const Board &board, Bitboard attacksMobility[2][8], Score& score) {
    int kingSafetyUnits[2] = {0}; //[COLOR]

    for(COLOR color : {WHITE, BLACK}) {
        COLOR enemyColor = (COLOR)!color;
        int kingSquare = BitscanForward( board.Piece(enemyColor, KING) );
        Bitboard kingRing = KING_INNER_RING[kingSquare] | KING_OUTER_RING[kingSquare];

        Bitboard enemyPawnAttacks = attacksMobility[enemyColor][PAWN];
        Bitboard pawnRestrictions = board.Piece(color, PAWN) | enemyPawnAttacks;

        Bitboard enemyAttacks = attacksMobility[enemyColor][ALL_PIECES];

        for(PIECE_TYPE pieceType = KNIGHT; pieceType <= QUEEN; ++pieceType) {

            Bitboard allowedAttacks = attacksMobility[color][pieceType] & kingRing & ~pawnRestrictions;
            if(!allowedAttacks)
                continue;

            //Checks and squares defended by lower-value pieces
            Bitboard checks = ZERO;
            Bitboard enemyAttacksLower = ZERO;
            switch(pieceType) {
                case KNIGHT:
                    checks = Attacks::AttacksKnights(kingSquare); break;
                case BISHOP:
                    checks = Attacks::AttacksSliding(pieceType, kingSquare, board.AllPieces()); break;
                case ROOK:
                    checks = Attacks::AttacksSliding(pieceType, kingSquare, board.AllPieces()); 
                    enemyAttacksLower = attacksMobility[enemyColor][KNIGHT] | attacksMobility[enemyColor][BISHOP]; break;
                case QUEEN:
                    checks = Attacks::AttacksSliding(pieceType, kingSquare, board.AllPieces()); 
                    enemyAttacksLower = attacksMobility[enemyColor][KNIGHT] | attacksMobility[enemyColor][BISHOP] | attacksMobility[enemyColor][ROOK]; break;
                default: assert(false);
            };

            kingSafetyUnits[color] += PopCount(allowedAttacks & ~enemyAttacks &  checks) * parameters.KS_BONUS_PIECETYPE[0][pieceType];
            kingSafetyUnits[color] += PopCount(allowedAttacks & ~enemyAttacks & ~checks) * parameters.KS_BONUS_PIECETYPE[1][pieceType];
            kingSafetyUnits[color] += PopCount(allowedAttacks &  enemyAttacks & ~enemyAttacksLower &  checks) * parameters.KS_BONUS_PIECETYPE[2][pieceType];
            kingSafetyUnits[color] += PopCount(allowedAttacks &  enemyAttacks & ~enemyAttacksLower & ~checks) * parameters.KS_BONUS_PIECETYPE[3][pieceType];
        }
    }

    EvalKingSafety_WeakFiles(board, kingSafetyUnits);
    EvalKingSafety_WeakSquares(board, kingSafetyUnits, attacksMobility);

    kingSafetyUnits[WHITE] /= 10;
    kingSafetyUnits[BLACK] /= 10;

    if(kingSafetyUnits[WHITE] > 127) kingSafetyUnits[WHITE] = 127;
    if(kingSafetyUnits[BLACK] > 127) kingSafetyUnits[BLACK] = 127;

    score.Add     ( KING_SAFETY_TABLE[ kingSafetyUnits[WHITE] ], 0 );
    score.Subtract( KING_SAFETY_TABLE[ kingSafetyUnits[BLACK] ], 0 );
}

//Assign a bonus if there are open/semiopen files near the enemy king, and we have a rook
void Evaluation::EvalKingSafety_WeakFiles(const Board& board, int kingSafetyUnits[2]) {
    for(COLOR color : {WHITE, BLACK}) {
        if( board.Piece(color, ROOK) ) {
            COLOR enemyColor = (COLOR)!color;
            Bitboard enemyKing = board.Piece(enemyColor, KING);
            int kingSquare = BitscanForward(enemyKing);
            int kingFile = File(kingSquare);

            //King file
            bool isSemiopen = false;
            if( IsSemiopenFile(board, color, kingSquare) ) {
                isSemiopen = true;
                kingSafetyUnits[color] += parameters.KS_KING_SEMIOPEN[0];
            }
            if( IsSemiopenFile(board, enemyColor, kingSquare) ) {
                kingSafetyUnits[color] += parameters.KS_KING_SEMIOPEN[1];
                if(isSemiopen)
                    kingSafetyUnits[color] += parameters.KS_KING_OPEN;
            }
            //Adjacent left: missing pawn
            if( kingFile != FILEA && IsSemiopenFile(board, enemyColor, kingSquare - 1) )
                kingSafetyUnits[color] += parameters.KS_KING_SEMIOPEN_ADJACENT;
            //Adjacent right: missing pawn
            if( kingFile != FILEH && IsSemiopenFile(board, enemyColor, kingSquare + 1) )
                kingSafetyUnits[color] += parameters.KS_KING_SEMIOPEN_ADJACENT;
        }//rook?
    } //color
}

//Assign a bonus if there are weak squares around the enemy king, and we have bishop and queen
//Weak square: not defended by pawns or bishops
void Evaluation::EvalKingSafety_WeakSquares(const Board& board, int kingSafetyUnits[2], Bitboard attacksMobility[2][8]) {
    for(COLOR color : {WHITE, BLACK}) {
        if( board.Piece(color,QUEEN) ) {
            COLOR enemyColor = (COLOR)!color;
            int kingSquare = BitscanForward( board.Piece(enemyColor, KING) );
            Bitboard kingRing = KING_INNER_RING[kingSquare] | KING_OUTER_RING[kingSquare];
            Bitboard controlledSqures = board.Piece(enemyColor, PAWN) | attacksMobility[enemyColor][PAWN]
                                        | board.Piece(enemyColor, BISHOP) | attacksMobility[enemyColor][BISHOP];
            
            if(board.Piece(color, BISHOP) & DARK_SQUARES) {
                int weaknesses = PopCount(kingRing & ~controlledSqures & DARK_SQUARES);
                kingSafetyUnits[color] += calculations.KS_WEAK_SQUARES[weaknesses];
            }
            if(board.Piece(color, BISHOP) & LIGHT_SQUARES) {
                int weaknesses = PopCount(kingRing & ~controlledSqures & LIGHT_SQUARES);
                kingSafetyUnits[color] += calculations.KS_WEAK_SQUARES[weaknesses];
            }
        } //if queen
    } //color
}

void Evaluation::EvalMaterial(const Board& board, Score& score) {
    for(PIECE_TYPE pieceType : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN}) {
        int countBalance = PopCount( board.Piece(WHITE,pieceType) ) - PopCount( board.Piece(BLACK,pieceType) );
        score.Add(
            countBalance * parameters.MATERIAL_VALUES[MG][pieceType],
            countBalance * parameters.MATERIAL_VALUES[EG][pieceType]
        );
    }
}

//Try to retrieve the pawn structure from hash. If not found, perform the calculation
void Evaluation::EvalPawns(const Board &board, Score& score) {

    //Debug
    D(
        if(test_total % 10000000 == 0) {
            P("PawnKey: " << board.PawnKey());
            P("PawnHash hitrate: calls " << test_total \
                        << ", hit " << test_hit \
                        << ", miss " << test_miss \
                        << ", rate " << 100 * (float)test_hit / test_total << "%" \
                        << ", fill " << 100 * Hash::pawnHash.Occupancy() << "%");
        }
        test_total++;
    );

    PawnEntry* pawnEntry = Hash::pawnHash.ProbeEntry( board.PawnKey() );
    if(pawnEntry) {
        D(test_hit++);

        score.Add(pawnEntry->evalMg, pawnEntry->evalEg);
    }
    else {
        D(test_miss++);

        //Calculate
        TaperedScore whiteEval = EvalPawnsCalculation(board, WHITE);
        TaperedScore blackEval = EvalPawnsCalculation(board, BLACK);
        int scoreMg = whiteEval.mg - blackEval.mg;
        int scoreEg = whiteEval.eg - blackEval.eg;

        //Store in hash
        Hash::pawnHash.AddEntry(board.PawnKey(), scoreMg, scoreEg);

        score.Add(scoreMg, scoreEg);
    }
}

TaperedScore Evaluation::EvalPawnsCalculation(const Board &board, COLOR color) {
    TaperedScore score;

    Bitboard thePawns = board.Piece(color, PAWN);
    Bitboard enemyPawns = board.Piece((COLOR)!color, PAWN);

    //--Double pawns
    //1-rank distance
    int doubledPawns = PopCount(thePawns & North(thePawns));
    score.mg += doubledPawns * parameters.DOUBLED_PAWN[MG];
    score.eg += doubledPawns * parameters.DOUBLED_PAWN[EG];
    //2-rank distance: half bonus
    doubledPawns = PopCount(thePawns & North(thePawns, 2));
    score.mg += doubledPawns * parameters.DOUBLED_PAWN[MG] / 2;
    score.eg += doubledPawns * parameters.DOUBLED_PAWN[EG] / 2;

    Bitboard bb = thePawns;
    while(bb) {
        int square = ResetLsb(bb);

        //Psqt
        int index = SQUARE_CONVERSION[color][square];
        score.mg += parameters.PSQT[PAWN][index];
        score.eg += parameters.PSQT_ENDGAME[PAWN][index];

        int file = File(square);
        bool isPassed = !(PASSED_PAWN_AREA[color][square] & enemyPawns);
        bool isIsolated = !(thePawns & ADJACENT_FILES[file]);

        if(isPassed) {
            int rank = RelativeRank(color, square);
            score.mg += parameters.PASSED_PAWN[MG][rank];
            score.eg += parameters.PASSED_PAWN[EG][rank];
        }
        if(isIsolated) {
            int rank = RelativeRank(color, square);
            score.mg += parameters.ISOLATED_PAWN[MG][rank];
            score.eg += parameters.ISOLATED_PAWN[EG][rank];
        }
    }

    return score;
}

TaperedScore Evaluation::EvalRookOpen(const Board& board, COLOR color) {
    TaperedScore score;
    Bitboard bb = board.Piece(color, ROOK);
    while(bb) {
        int square = ResetLsb(bb);
        //Semi-open files
        if( IsSemiopenFile(board, color, square) ) {
            score.mg += parameters.ROOK_SEMIOPEN[MG];
            score.eg += parameters.ROOK_SEMIOPEN[EG];
            if( IsSemiopenFile(board, (COLOR)!color, square) ) {
                score.mg += parameters.ROOK_OPEN[MG];
                score.eg += parameters.ROOK_OPEN[EG];
            }
        }
    }
    return score;
}

int Evaluation::ClassicalEvaluation(const Board& board) {
    Score score;
    Bitboard attacksMobility[2][8] = {{0}}; //[COLOR][PIECE_TYPE]

    //Automatic draws
    if( InsufficientMaterial(board) )
        return 0;
        
    EvalMaterial(board, score);

    PawnAttacks(board, attacksMobility);

    //Psqt and mobility
    for(COLOR color : {WHITE, BLACK}) {
        const int sign = color == WHITE ? 1 : -1;

        //Pawn restrictions for mobility and king safety
        Bitboard ownPawns = board.Piece(color, PAWN);
        Bitboard enemyPawnAttacks = attacksMobility[(COLOR)!color][PAWN];
        Bitboard pawnRestrictions = ownPawns | enemyPawnAttacks;

        for(PIECE_TYPE pieceType = KNIGHT; pieceType <= KING; ++pieceType) {
              
            Bitboard bb = board.Piece(color, pieceType);
            while(bb) {
                int square = ResetLsb(bb);

                //Psqt
                int index = SQUARE_CONVERSION[color][square];
                score.Add(
                    sign * parameters.PSQT[pieceType][index],
                    sign * parameters.PSQT_ENDGAME[pieceType][index]
                );

                //Mobility
                if(pieceType == KNIGHT) {
                    Bitboard attacks = Attacks::AttacksKnights(square);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * calculations.MOBILITY_KNIGHT[MG][mob],
                        sign * calculations.MOBILITY_KNIGHT[EG][mob]
                    );
                }
                else if(pieceType == BISHOP) {
                    Bitboard blockers = board.AllPieces() ^ (board.Piece(color, BISHOP) | board.Piece(color, QUEEN));
                    Bitboard attacks = Attacks::AttacksSliding(BISHOP, square, blockers);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * calculations.MOBILITY_BISHOP[MG][mob],
                        sign * calculations.MOBILITY_BISHOP[EG][mob]
                    );
                }
                else if(pieceType == ROOK) {
                    Bitboard blockers = board.AllPieces() ^ (board.Piece(color, ROOK) | board.Piece(color, QUEEN));
                    Bitboard attacks = Attacks::AttacksSliding(ROOK, square, blockers);
                    attacksMobility[color][pieceType] |= attacks;
                    attacksMobility[color][ALL_PIECES] |= attacks;
                    int mob = PopCount(attacks & ~pawnRestrictions);
                    score.Add(
                        sign * calculations.MOBILITY_ROOK[MG][mob],
                        sign * calculations.MOBILITY_ROOK[EG][mob]
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
                        sign * calculations.MOBILITY_QUEEN[MG][mob],
                        sign * calculations.MOBILITY_QUEEN[EG][mob]
                    );
                }
            } //piece
        } //pieceType
    } //color

    EvalKingSafety(board, attacksMobility, score);
    EvalPawns(board, score);

    score.Add     ( EvalRookOpen(board, WHITE) );
    score.Subtract( EvalRookOpen(board, BLACK) );
    
    score.Add     ( EvalBishopPair(board, WHITE) );
    score.Subtract( EvalBishopPair(board, BLACK) );

    const int sign = board.ActivePlayer() == WHITE ? 1 : -1;
    return sign * score.Tapered( Phase(board) );
}

int Evaluation::Evaluate(const Board& board) {
    //Automatic draw
    if( InsufficientMaterial(board) )
        return 0;

    int color = board.ActivePlayer();
    int eval = nnue.Evaluate(color);

    return eval;
}
