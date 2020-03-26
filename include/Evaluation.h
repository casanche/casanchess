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

    //King Safety formulas
    //S-shaped function that starts at zero and grows until the maximum
    inline double Sigmoid(double x, double maximum, double midPoint, double slope) {
        //f(x) = g(x) - g(0), where g(x) = a / (1 + exp(-c * (x-b)) )
        return maximum / (1 + exp( -slope * (x - midPoint) ) )
             - maximum / (1 + exp( -slope * (0 - midPoint) ) ); 
    }

    //Mobility formulas
    #define LINEAR(a, b) (std::lrint( a + b*mob )) //a + bx
    #define QUADRATIC(a, b, c) (std::lrint( a + b*mob + c*mob*mob )) //a + bx + cx^2
    #define SCALED_CUBIC(f, a, b, c, d) (std::lrint( f*(a + b*mob + c*mob*mob + d*mob*mob*mob) )) //f*(a + bx + cx^2 + dx^3)

    template<GAME_PHASE ph> constexpr int MOB_N(U8 mob) {
        if constexpr(ph == MIDDLEGAME) return SCALED_CUBIC(1, -25.3, 4.53, -0.67, 0.0715);
        else                           return (mob < 3) * LINEAR(-41.7, 4.0) + (mob >= 3) * QUADRATIC(-63.6, 19.8, -1.34);
    }
    template<GAME_PHASE ph> constexpr int MOB_B(U8 mob) {
        if constexpr(ph == MIDDLEGAME) return SCALED_CUBIC(1, -50.9, 17.8, -2, 0.0877);
        else                           return SCALED_CUBIC(1, -95, 26.5, -2.6, 0.0955);
    }
    template<GAME_PHASE ph> constexpr int MOB_R(U8 mob) {
        if constexpr(ph == MIDDLEGAME) return SCALED_CUBIC(1, -14.5, 0.87, 0.29, -0.0095);
        else                           return SCALED_CUBIC(1, -58.6, +12.2, -0.59, +0.0041);
    }
    template<GAME_PHASE ph> constexpr int MOB_Q(U8 mob) {
        if constexpr(ph == MIDDLEGAME) return SCALED_CUBIC(0.5, -29.5, 2.63, 0.026, -0.002);
        else                           return SCALED_CUBIC(0.5, -27.9, 2.29, 0.026, -0.0018);
    }

    //Precomputed tables
    extern Bitboard ADJACENT_FILES[8]; //[FILE]
    extern Bitboard ADJACENT_RANKS[8]; //[RANK]
    extern Bitboard PASSED_PAWN_FRONT[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_SIDES[2][64]; //[COLOR][SQUARE]
    extern Bitboard PASSED_PAWN_AREA[2][64]; //[COLOR][SQUARE]
    extern Bitboard KING_INNER_RING[64]; //[SQUARE]
    extern Bitboard KING_OUTER_RING[64]; //[SQUARE]
    extern int KING_SAFETY_TABLE[128]; //[KING_SAFETY_POINTS]
    void Init();

    int Evaluate(const Board& board);

    bool AreHeavyPieces(const Board& board);
    bool InsufficientMaterial(const Board &board);
    bool IsSemiopenFile(const Board& board, COLORS color, int square);
    void PawnAttacks(const Board& board, Bitboard attacksMobility[2][8]);
    int Phase(const Board& board);

    TaperedScore EvalBishopPair(const Board &board, COLORS color);
    void EvalKingSafety(const Board &board, Bitboard attacksMobility[2][8], Score& score);
    void EvalKingSafety_RookOpen(const Board& board, int (&kingSafetyUnits)[2]);
    void EvalMaterial(const Board& board, Score& score);
    void EvalPawns(const Board& board, Score& score);
    TaperedScore EvalPawnsCalculation(const Board& board, COLORS color);
    TaperedScore EvalRookOpen(const Board& board, COLORS color);

    //Evaluation parameters for tuning
    const struct Parameters {
        int MATERIAL_VALUES[2][8] = {
            {0, 98, 397, 385, 490, 1150, 0},
            {0, 94, 345, 360, 600, 1150, 0}
        };

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

        int KS_KING_SEMIOPEN[2] = {20, 60}; //Semiopen for [COLOR]
        int KS_KING_OPEN = 60;
        int KS_KING_SEMIOPEN_ADJACENT = 40;

        int BISHOP_PAIR[2] = {35, 52};

        int MOBILITY_KNIGHT[2][9] = {
            {MOB_N<MG>(0), MOB_N<MG>(1), MOB_N<MG>(2), MOB_N<MG>(3), MOB_N<MG>(4), MOB_N<MG>(5), MOB_N<MG>(6), MOB_N<MG>(7), MOB_N<MG>(8)},
            {MOB_N<EG>(0), MOB_N<EG>(1), MOB_N<EG>(2), MOB_N<EG>(3), MOB_N<EG>(4), MOB_N<EG>(5), MOB_N<EG>(6), MOB_N<EG>(7), MOB_N<EG>(8)}
        };
        int MOBILITY_BISHOP[2][14] = {
            {MOB_B<MG>(0), MOB_B<MG>(1), MOB_B<MG>(2), MOB_B<MG>(3), MOB_B<MG>(4), MOB_B<MG>(5), MOB_B<MG>(6), MOB_B<MG>(7), MOB_B<MG>(8), MOB_B<MG>(9), MOB_B<MG>(10), MOB_B<MG>(11), MOB_B<MG>(12), MOB_B<MG>(13)},
            {MOB_B<EG>(0), MOB_B<EG>(1), MOB_B<EG>(2), MOB_B<EG>(3), MOB_B<EG>(4), MOB_B<EG>(5), MOB_B<EG>(6), MOB_B<EG>(7), MOB_B<EG>(8), MOB_B<EG>(9), MOB_B<EG>(10), MOB_B<EG>(11), MOB_B<EG>(12), MOB_B<EG>(13)}
        };
        int MOBILITY_ROOK[2][15] = {
            {MOB_R<MG>(0), MOB_R<MG>(1), MOB_R<MG>(2), MOB_R<MG>(3), MOB_R<MG>(4), MOB_R<MG>(5), MOB_R<MG>(6), MOB_R<MG>(7), MOB_R<MG>(8), MOB_R<MG>(9), MOB_R<MG>(10), MOB_R<MG>(11), MOB_R<MG>(12), MOB_R<MG>(13), MOB_R<MG>(14)},
            {MOB_R<EG>(0), MOB_R<EG>(1), MOB_R<EG>(2), MOB_R<EG>(3), MOB_R<EG>(4), MOB_R<EG>(5), MOB_R<EG>(6), MOB_R<EG>(7), MOB_R<EG>(8), MOB_R<EG>(9), MOB_R<EG>(10), MOB_R<EG>(11), MOB_R<EG>(12), MOB_R<EG>(13), MOB_R<MG>(14)}
        };
        int MOBILITY_QUEEN[2][28] = {
            {MOB_Q<MG>(0), MOB_Q<MG>(1), MOB_Q<MG>(2), MOB_Q<MG>(3), MOB_Q<MG>(4), MOB_Q<MG>(5), MOB_Q<MG>(6), MOB_Q<MG>(7), MOB_Q<MG>(8), MOB_Q<MG>(9), MOB_Q<MG>(10), MOB_Q<MG>(11), MOB_Q<MG>(12), MOB_Q<MG>(13), MOB_Q<MG>(14), MOB_Q<MG>(15), MOB_Q<MG>(16), MOB_Q<MG>(17), MOB_Q<MG>(18), MOB_Q<MG>(19), MOB_Q<MG>(20), MOB_Q<MG>(21), MOB_Q<MG>(22), MOB_Q<MG>(23), MOB_Q<MG>(24), MOB_Q<MG>(25), MOB_Q<MG>(26), MOB_Q<MG>(27)},
            {MOB_Q<EG>(0), MOB_Q<EG>(1), MOB_Q<EG>(2), MOB_Q<EG>(3), MOB_Q<EG>(4), MOB_Q<EG>(5), MOB_Q<EG>(6), MOB_Q<EG>(7), MOB_Q<EG>(8), MOB_Q<EG>(9), MOB_Q<EG>(10), MOB_Q<EG>(11), MOB_Q<EG>(12), MOB_Q<EG>(13), MOB_Q<EG>(14), MOB_Q<EG>(15), MOB_Q<EG>(16), MOB_Q<EG>(17), MOB_Q<EG>(18), MOB_Q<EG>(19), MOB_Q<EG>(20), MOB_Q<EG>(21), MOB_Q<EG>(22), MOB_Q<EG>(23), MOB_Q<EG>(24), MOB_Q<EG>(25), MOB_Q<EG>(26), MOB_Q<EG>(27)}
        };
    } params;

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
