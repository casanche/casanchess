#ifndef EVALUATION_H
#define EVALUATION_H

#include "Constants.h"
#include "Hash.h"
#include <cmath>

class Board;

namespace Evaluation {

    enum GAME_PHASE { MIDDLEGAME, ENDGAME };
    #define MG MIDDLEGAME
    #define EG ENDGAME

    struct TaperedScore {
        int mg;
        int eg;
        TaperedScore() : mg(0), eg(0) {}
        TaperedScore(int mgScore, int egScore) : mg(mgScore), eg(egScore) {}
    };

    class Score;

    //Precomputed tables
    extern Bitboard ADJACENT_FILES[8]; //[FILE]
    extern Bitboard ADJACENT_RANKS[8]; //[RANK]
    extern Bitboard PASSED_PAWN_FRONT[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_SIDES[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_AREA[2][64]; //[COLOR][SQUARE]
    extern Bitboard KING_INNER_RING[64]; //[SQUARE]
    extern Bitboard KING_OUTER_RING[64]; //[SQUARE]
    extern u16 KING_SAFETY_TABLE[128]; //[KING_SAFETY_POINTS]
    void Init();

    //Helper methods
    bool AreHeavyPieces(const Board& board);
    bool InsufficientMaterial(const Board &board);
    bool IsSemiopenFile(const Board& board, COLOR color, int square);
    void PawnAttacks(const Board& board, Bitboard attacksMobility[2][8]);
    int Phase(const Board& board);

    //Evaluate functions
    int ClassicalEvaluation(const Board& baord);
    int Evaluate(const Board& board);
    TaperedScore EvalBishopPair(const Board &board, COLOR color);
    void EvalKingSafety(const Board &board, Bitboard attacksMobility[2][8], Score& score);
    void EvalKingSafety_WeakFiles(const Board& board, int kingSafetyUnits[2]);
    void EvalKingSafety_WeakSquares(const Board& board, int kingSafetyUnits[2], Bitboard attacksMobility[2][8]);
    void EvalMaterial(const Board& board, Score& score);
    void EvalPawns(const Board& board, Score& score);
    TaperedScore EvalPawnsCalculation(const Board& board, COLOR color);
    TaperedScore EvalRookOpen(const Board& board, COLOR color);

    //Evaluation parameters for tuning
    const struct Parameters {
        int MATERIAL_VALUES[2][8] = {
            {0, 98, 397, 385, 490, 1150, 0},
            {0, 94, 345, 360, 600, 1150, 0}
        };
        int BISHOP_PAIR[2] = {35, 52};

        int DOUBLED_PAWN[2] = {-15, -12};
        int PASSED_PAWN[2][8] = {  //[RANK]
            {0,  0,  0,  6, 18,  35,  50, 0},
            {0, 10, 18, 40, 55, 125, 185, 0}
        };
        int ISOLATED_PAWN[2][8] = {
            {0, -15, -12, -20, -15, -10, -5, 0},
            {0, -15, -12, -20, -15, -10, -5, 0}
        };

        int ROOK_SEMIOPEN[2] = {26, 8};
        int ROOK_OPEN[2] = {32, -7};

        int KS_SIGMOID[3] = {315, 38, 75};
        int KS_KING_SEMIOPEN[2] = {20, 60}; //Semiopen for [COLOR]
        int KS_KING_OPEN = 60;
        int KS_KING_SEMIOPEN_ADJACENT = 40;
        int KS_BONUS_PIECETYPE[4][6] = { //[][PIECE_TYPE]
            {0, 0, 220, 140, 140, 125}, //Undefended squares, checks
            {0, 0, 44, 31, 5, 85}, //Undefended squares
            {0, 0, 75, 65, 86, 60}, //Defended by pieces of equal-or-higher value, checks
            {0, 0, 55, 10, 50, 45} //Defended by pieces of equal-or-higher value
        };
        int KS_WEAK_SQUARES_GROW = 75;

        i8 PSQT[8][64] = {
            //NO_PIECE
            {0},
            //PAWN
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                50, 50, 50, 50, 50, 50, 50, 50, // 8...R7
                12, 12, 20, 30, 30, 20, 12, 12, //16...R6
                5,  10, 12, 28, 28, 12, 10, 5,  //24...R5
                0,  0,  0,  23, 23, 0,  0,  0,  //32...R4
                2,  5,  2,  0,  0,  2,  5,  2,  //40...R3
                0,  5,  5, -15,-15, 5,  5,  0,  //48...R2
                0,  0,  0,  0,  0,  0,  0,  0
            },
            //KNIGHT
            {
                -50,-40,-30,-30,-30,-30,-40,-50,
                -40,-20,  0,  0,  0,  0,-20,-40,
                -30,  0, 10, 15, 15, 10,  0,-30,
                -30,  5, 15, 20, 20, 15,  5,-30,
                -30,  0, 15, 20, 20, 15,  0,-30,
                -30,  5, 10, 15, 15, 10,  5,-30,
                -40,-20,  0,  5,  5,  0,-20,-40,
                -50,-40,-30,-30,-30,-30,-40,-50
            },
            //BISHOP
            {
                -20,-10,-10,-10,-10,-10,-10,-20,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -10,  0,  5, 10, 10,  5,  0,-10,
                -10,  5,  5, 10, 10,  5,  5,-10,
                -10,  0, 10, 10, 10, 10,  0,-10,
                -10, 10, 10, 12, 12, 10, 10,-10,
                -10,  5,  0,  0,  0,  0,  5,-10,
                -20,-10,-12,-10,-10,-12,-10,-20
            },
            //ROOK
            {
                0,  0,  0,  0,  0,  0,  0,  0,
                5, 10, 10, 10, 10, 10, 10,  5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -5,  0,  0,  0,  0,  0,  0, -5,
                -8,  0,  0,  0,  0,  0,  0, -8,
                0,  0,  5,  5,  5,  5,  0,  0
            },
            //QUEEN
            {
                -20,-10,-10, -5, -5,-10,-10,-20,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -10,  0,  5,  5,  5,  5,  0,-10,
                -5,   0,  5,  5,  5,  5,  0, -5,
                -5,   0,  5,  5,  5,  5,  0, -5,
                -10,  5,  5,  5,  5,  5,  5,-10,
                -10,  0,  0,  0,  0,  0,  0,-10,
                -20,-10,-10, -5, -5,-10,-10,-20
            },
            //KING
            {
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -30,-40,-40,-50,-50,-40,-40,-30,
                -20,-30,-30,-40,-40,-30,-30,-20,
                -10,-20,-20,-25,-25,-20,-20,-10,
                20, 22,  0,-15,-15,  0, 22, 20,
                20, 38,  3,  0,  0,  3, 38, 20
            }
        };

        i8 PSQT_ENDGAME[8][64] = {
            {0},
            //PAWN
            {
                0,   0,  0,  0,  0,  0,  0,  0,
                50, 50, 50, 50, 50, 50, 50, 50, // 8...R7
                25, 15, 20, 15, 15, 20, 15, 25, //16...R6
                17, 15, 10,  2,  2, 10, 15, 17, //24...R5
                13,  7,  0, -2, -2,  0,  7, 13, //32...R4
                0,   0, -2,  2,  2, -2,  0,  0,  //40...R3
                0,   0,  8, -2, -2,  8,  0,  0,  //48...R2
                0,   0,  0,  0,  0,  0,  0,  0
            },
            { *PSQT[KNIGHT] },
            { *PSQT[BISHOP] },
            { *PSQT[ROOK] },
            { *PSQT[QUEEN] },
            //KING
            {
                -50,-40,-30,-20,-20,-30,-40,-50,
                -30,-20,-10,  0,  0,-10,-20,-30,
                -30,-10, 20, 30, 30, 20,-10,-30,
                -20, -5, 30, 32, 32, 30, -5,-20,
                -20, -5, 22, 30, 30, 22, -5,-20,
                -30, -5, 13, 22, 22, 13, -5,-30,
                -30,-23,  0,  2,  2,  0,-23,-30,
                -55,-30,-30,-30,-30,-30,-30,-55
            }
        };

    } parameters;

    //Evaluation bonuses that require calculation
    const struct Calculations {
        Calculations() {
            for(GAME_PHASE ph : {MG, EG}) {
                for(int mob = 0; mob <  9; mob++) MOBILITY_KNIGHT[ph][mob] = SafeCastInt16(Mobility(KNIGHT, ph, mob));
                for(int mob = 0; mob < 14; mob++) MOBILITY_BISHOP[ph][mob] = SafeCastInt16(Mobility(BISHOP, ph, mob));
                for(int mob = 0; mob < 15; mob++) MOBILITY_ROOK[ph][mob]   = SafeCastInt16(Mobility(ROOK,   ph, mob));
                for(int mob = 0; mob < 28; mob++) MOBILITY_QUEEN[ph][mob]  = SafeCastInt16(Mobility(QUEEN, ph, mob));
            }
            for(int x = 0; x < 13; x++) KS_WEAK_SQUARES[x] = WeakSquares(x);
        };

        #define LINEAR(a, b) (std::lrint( a + b*mob )) //a + bx
        #define QUADRATIC(a, b, c) (std::lrint( a + b*mob + c*mob*mob )) //a + bx + cx^2
        #define SCALED_CUBIC(f, a, b, c, d) (std::lrint( f*(a + b*mob + c*mob*mob + d*mob*mob*mob) )) //f*(a + bx + cx^2 + dx^3)
        
        int Mobility(PIECE_TYPE pieceType, GAME_PHASE ph, int mob) {
            if(ph == MG) {
                switch(pieceType) {
                case KNIGHT:    return SCALED_CUBIC(1, -25.3, 4.53, -0.67, 0.0715); break;
                case BISHOP:    return SCALED_CUBIC(1, -50.9, 17.8, -2, 0.0877); break;
                case ROOK:      return SCALED_CUBIC(1, -14.5, 0.87, 0.29, -0.0095); break;
                case QUEEN:     return SCALED_CUBIC(0.5, -29.5, 2.63, 0.026, -0.002); break;
                default: assert(false);
                };
            }
            else if(ph == EG) {
                switch(pieceType) {
                case KNIGHT:    return (mob < 3) * LINEAR(-41.7, 4.0) + (mob >= 3) * QUADRATIC(-63.6, 19.8, -1.34); break;
                case BISHOP:    return SCALED_CUBIC(1, -95, 26.5, -2.6, 0.0955); break;
                case ROOK:      return SCALED_CUBIC(1, -58.6, +12.2, -0.59, +0.0041); break;
                case QUEEN:     return SCALED_CUBIC(0.5, -27.9, 2.29, 0.026, -0.0018); break;
                default: assert(false);
                };
            }
            assert(false);
            return 0;
        }
        i16 WeakSquares(int x) {
            if(x < 2) return 0;
            if(x > 6) x = 6;
            double result = parameters.KS_WEAK_SQUARES_GROW * sqrt(x-2);
            return SafeCastInt16(static_cast<int>(result));
        }

        i16 MOBILITY_KNIGHT[2][9];
        i16 MOBILITY_BISHOP[2][14];
        i16 MOBILITY_ROOK[2][15];
        i16 MOBILITY_QUEEN[2][28];
        i16 KS_WEAK_SQUARES[13];
    } calculations;

} //namespace Evaluation

#endif //EVALUATION_H
