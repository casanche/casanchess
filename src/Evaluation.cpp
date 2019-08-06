#include "Evaluation.h"
using namespace Evaluation;

#include "Attacks.h"
#include "Board.h"
#include "BitboardUtils.h"

//Constants
#define MG MIDDLEGAME
#define EG ENDGAME
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

//Precomputed tables
Bitboard Evaluation::ADJACENT_FILES[8] = {0};
Bitboard Evaluation::ADJACENT_RANKS[8] = {0};
Bitboard Evaluation::PASSED_PAWN_FRONT[2][64] = {{0}};
Bitboard Evaluation::PASSED_PAWN_SIDES[2][64] = {{0}};
Bitboard Evaluation::PASSED_PAWN_AREA[2][64] = {{0}};

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
}

bool Evaluation::AreHeavyPiecesOnBothSides(const Board& board) {
    return board.Piece(WHITE,ALL_PIECES) ^ (board.Piece(WHITE,PAWN) | board.Piece(WHITE,KING))
        && board.Piece(BLACK,ALL_PIECES) ^ (board.Piece(BLACK,PAWN) | board.Piece(BLACK,KING));
}

bool Evaluation::IsSemiopenFile(const Board& board, COLORS color, int square) {
    return !( board.Piece(color, PAWN) & MaskFile[ File(square) ] );
}

TaperedScore Evaluation::EvalBishopPair(const Board &board, COLORS color) {
    TaperedScore score;
    if( (board.Piece(color, BISHOP) & LIGHT_SQUARES) && (board.Piece(color, BISHOP) & DARK_SQUARES) ) {
        score.mg = params.BISHOP_PAIR[MG];
        score.eg = params.BISHOP_PAIR[EG];
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

TaperedScore Evaluation::EvalPawns(const Board &board, COLORS color) {
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
        score.eg += PSQT[PAWN][index];

        int file = File(square);
        // Bitboard pushSquare = color == WHITE ? North(SquareBB(square)) : South(SquareBB(square));
        bool isPassed = !(PASSED_PAWN_AREA[color][square] & enemyPawns);
        // bool isDoubled = thePawns & PASSED_PAWN_FRONT[color][square]; //instead of pushSquare
        bool isIsolated = !(thePawns & ADJACENT_FILES[file]);
        // bool isBackward = enemyPawnsAttacks & pushSquare;
        // bool isBackward = thePawns & ADJACENT_FILES[file];

        if(isPassed) {
            int rank = ColorlessRank(color, square);

            //Check if the front span is not attacked by enemy pieces
            bool isFreePassed = true;
            Bitboard frontSpan = PASSED_PAWN_FRONT[color][square];
            while(frontSpan) {
                int spanSq = ResetLsb(frontSpan);
                //There is an enemy piece IN the square
                bool thereIsAPiece = SquareBB(spanSq) & board.Piece((COLORS)!color, ALL_PIECES);
                //There are enemy pieces ATTACKING the square
                Bitboard attackers = board.AttackersTo(color, spanSq);
                if(attackers || thereIsAPiece) {
                    isFreePassed = false;
                    break;
                }
            }

            if(isFreePassed) {
                score.mg += params.FREE_PASSED_PAWN[MG][rank];
                score.eg += params.FREE_PASSED_PAWN[EG][rank];
            }
            else {
                score.mg += params.PASSED_PAWN[MG][rank];
                score.eg += params.PASSED_PAWN[EG][rank];
            }
        }
        if(isIsolated) {
            int rank = ColorlessRank(color, square);
            score.mg += params.ISOLATED_PAWN[MG][rank];
            score.eg += params.ISOLATED_PAWN[EG][rank];
        }
    }

    return score;
}

#ifdef DEBUG_PAWN_HASH
int test_total = 0;
int test_hit = 0;
int test_miss = 0;
#endif


//Try to retrieve the pawn structure from hash. If not found, calculate evaluation for the first time
TaperedScore Evaluation::EvalPawnsFromHash(const Board &board) {
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

        TaperedScore whiteEval = EvalPawns(board, WHITE);
        TaperedScore blackEval = EvalPawns(board, BLACK);
        TaperedScore score;
        score.mg = whiteEval.mg - blackEval.mg;
        score.eg = whiteEval.eg - blackEval.eg;

        //Store in hash
        Hash::pawnHash.AddEntry(board.PawnKey(), score.mg, score.eg);

        return score;
    }
}

int Evaluation::Evaluate(const Board& board) {
    Score score;
    int phase = 0;

    //Pawn material
    int whitePawns = PopCount(board.Piece(WHITE,PAWN));
    int blackPawns = PopCount(board.Piece(BLACK,PAWN));
    score.Add( (whitePawns-blackPawns) * params.MATERIAL_VALUES[MG][PAWN],
               (whitePawns-blackPawns) * params.MATERIAL_VALUES[EG][PAWN] );

    //Heavy material, psqt and phase
    TaperedScore heavyMaterial[2];
    for(COLORS color : {WHITE, BLACK}) {
        const int sign = color == WHITE ? 1 : -1;
        for(PIECE_TYPE pieceType = KNIGHT; pieceType <= KING; ++pieceType) {
            Bitboard bb = board.Piece(color, pieceType);
            int popcnt = PopCount(bb);
            //Material
            heavyMaterial[color].mg += params.MATERIAL_VALUES[MG][pieceType] * popcnt;
            heavyMaterial[color].eg += params.MATERIAL_VALUES[EG][pieceType] * popcnt;
            //Phase
            phase += PHASE_WEIGHT[pieceType] * popcnt;
            //Psqt
            while(bb) {
                int square = ResetLsb(bb);
                int index = SQUARE_CONVERSION[color][square];
                score.Add(sign * PSQT[pieceType][index], sign * PSQT_ENDGAME[pieceType][index]);
            }
        }
    }
    score.Add     (heavyMaterial[WHITE]);
    score.Subtract(heavyMaterial[BLACK]);

    //Insufficient material
    if(!whitePawns && !blackPawns && (heavyMaterial[WHITE].mg + heavyMaterial[BLACK].mg) < 400)
        return 0;

    //Pawns
    score.Add( EvalPawnsFromHash(board) );

    //Rook open files
    score.Add     ( EvalRookOpen(board, WHITE) );
    score.Subtract( EvalRookOpen(board, BLACK) );
    
    //Bishop pair
    score.Add     ( EvalBishopPair(board, WHITE) );
    score.Subtract( EvalBishopPair(board, BLACK) );

    const int sign = board.ActivePlayer() == WHITE ? 1 : -1;
    return sign * score.Tapered(phase);
}
