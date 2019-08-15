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

    //Mobility formulas
    template<GAME_PHASE ph>
    constexpr int MOB_N(U8 mob) {
        if constexpr(ph == MIDDLEGAME)
            return -25.3 + 4.53*mob - 0.67*mob*mob - 0.0715*mob*mob*mob;
        else
            return -33.3 + 2.96*mob + 1.42*mob*mob - 0.141*mob*mob*mob;
    }
    template<GAME_PHASE ph>
    constexpr int MOB_B(U8 mob) {
        if constexpr(ph == MIDDLEGAME)
            return -50.9 + 17.8*mob - 2*mob*mob + 0.0877*mob*mob*mob;
        else
            return -95 + 26.5*mob - 2.6*mob*mob + 0.0955*mob*mob*mob;
    }
    template<GAME_PHASE ph>
    constexpr int MOB_R(U8 mob) {
        if constexpr(ph == MIDDLEGAME)
            return std::lrint(-14.5 + 0.87*mob + 0.29*mob*mob - 0.0095*mob*mob*mob);
        else
            return std::lrint(-58.6 + 12.2*mob - 0.59*mob*mob + 0.0041*mob*mob*mob);
    }

    //Precomputed tables
    extern Bitboard ADJACENT_FILES[8]; //[FILE]
    extern Bitboard ADJACENT_RANKS[8]; //[RANK]
    extern Bitboard PASSED_PAWN_FRONT[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_SIDES[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_AREA[2][64]; //[COLOR][SQUARE]
    void Init();

    int Evaluate(const Board& board);

    bool AreHeavyPiecesOnBothSides(const Board& board);
    bool IsSemiopenFile(const Board& board, COLORS color, int square);

    TaperedScore EvalRookOpen(const Board& board, COLORS color);
    TaperedScore EvalBishopPair(const Board &board, COLORS color);
    
    TaperedScore EvalPawns(const Board& board);
    TaperedScore EvalPawnsCalculation(const Board& board, COLORS color);

    //Evaluation parameters for tuning
    const struct Parameters {
        int MATERIAL_VALUES[2][8] = {
            {0, 95, 380, 385, 500, 1120, 0},
            {0, 94, 340, 360, 600, 1120, 0}
        };

        int DOUBLED_PAWN[2] = {-15, -12};
        int PASSED_PAWN[2][8] = {  //[RANK]
            {0,  0,  0,  6, 20,  35,  50, 0},
            {0, 10, 18, 40, 55, 125, 185, 0}
        };
        int ISOLATED_PAWN[2][8] = {
            {0, -15, -12, -20, -15, -10, -5, 0},
            {0, -15, -12, -20, -15, -10, -5, 0}
        };

        int ROOK_SEMIOPEN[2] = {25, 10};
        int ROOK_OPEN[2] = {40, 0};
        int KING_SEMIOPEN[2] = {18, 0};
        int KING_OPEN[2] = {35, 0};
        int KING_SEMIOPEN_ADJACENT[2] = {8, -8};
        int KING_OPEN_ADJACENT[2] = {20, 0};

        int BISHOP_PAIR[2] = {35, 50};

        int MOBILITY_KNIGHT[2][9] = {
            {-22, -14, -7, -6, -4, 1, 10, 14, 22},
            {-42, -37, -34, -15, -7, 0, 9, 11, 8}
        };
        // int MOBILITY_KNIGHT[2][9] = {
        //     {MOB_N<MG>(0), MOB_N<MG>(1), MOB_N<MG>(2), MOB_N<MG>(3), MOB_N<MG>(4), MOB_N<MG>(5), MOB_N<MG>(6), MOB_N<MG>(7), MOB_N<MG>(8)},
        //     {MOB_N<EG>(0), MOB_N<EG>(1), MOB_N<EG>(2), MOB_N<EG>(3), MOB_N<EG>(4), MOB_N<EG>(5), MOB_N<EG>(6), MOB_N<EG>(7), MOB_N<EG>(8)}
        // };
        int MOBILITY_BISHOP[2][14] = {
            {MOB_B<MG>(0), MOB_B<MG>(1), MOB_B<MG>(2), MOB_B<MG>(3), MOB_B<MG>(4), MOB_B<MG>(5), MOB_B<MG>(6), MOB_B<MG>(7), MOB_B<MG>(8), MOB_B<MG>(9), MOB_B<MG>(10), MOB_B<MG>(11), MOB_B<MG>(12), MOB_B<MG>(13)},
            {MOB_B<EG>(0), MOB_B<EG>(1), MOB_B<EG>(2), MOB_B<EG>(3), MOB_B<EG>(4), MOB_B<EG>(5), MOB_B<EG>(6), MOB_B<EG>(7), MOB_B<EG>(8), MOB_B<EG>(9), MOB_B<EG>(10), MOB_B<EG>(11), MOB_B<EG>(12), MOB_B<EG>(13)}
        };
        int MOBILITY_ROOK[2][15] = {
            {MOB_R<MG>(0), MOB_R<MG>(1), MOB_R<MG>(2), MOB_R<MG>(3), MOB_R<MG>(4), MOB_R<MG>(5), MOB_R<MG>(6), MOB_R<MG>(7), MOB_R<MG>(8), MOB_R<MG>(9), MOB_R<MG>(10), MOB_R<MG>(11), MOB_R<MG>(12), MOB_R<MG>(13), MOB_R<MG>(14)},
            {MOB_R<EG>(0), MOB_R<EG>(1), MOB_R<EG>(2), MOB_R<EG>(3), MOB_R<EG>(4), MOB_R<EG>(5), MOB_R<EG>(6), MOB_R<EG>(7), MOB_R<EG>(8), MOB_R<EG>(9), MOB_R<EG>(10), MOB_R<EG>(11), MOB_R<EG>(12), MOB_R<EG>(13), MOB_R<MG>(14)}
        };
    } params;

    //from https://www.chessprogramming.org/Simplified_Evaluation_Function
    const int PSQT[8][64] = {
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

    const int PSQT_ENDGAME[8][64] = {
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

    const int SQUARE_CONVERSION[2][64] = { //[COLOR][SQUARE]
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

#endif //EVALUATION_H
