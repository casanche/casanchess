#ifndef EVALUATION_H
#define EVALUATION_H

#include "Constants.h"
#include "Hash.h"

class Board;

namespace Evaluation {

    enum GAME_PHASE { MIDDLEGAME, ENDGAME };

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

        int ROOK_SEMIOPEN[2] = {20, 10};
        int ROOK_OPEN[2] = {40, 0};
        int KING_SEMIOPEN[2] = {15, 0};
        int KING_OPEN[2] = {30, 0};
        int KING_SEMIOPEN_ADJACENT[2] = {5, -5};
        int KING_OPEN_ADJACENT[2] = {20, 10};

        int BISHOP_PAIR[2] = {35, 50};
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
            -10, 10, 10, 10, 10, 10, 10,-10,
            -10,  5,  0,  0,  0,  0,  5,-10,
            -20,-10,-10,-10,-10,-10,-10,-20
        },
        //ROOK
        {
            0,  0,  0,  0,  0,  0,  0,  0,
            5, 10, 10, 10, 10, 10, 10,  5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            0,  0,  0,  5,  5,  0,  0,  0
        },
        //QUEEN
        {
            -20,-10,-10, -5, -5,-10,-10,-20,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -10,  0,  5,  5,  5,  5,  0,-10,
            -5,  0,  5,  5,  5,  5,  0, -5,
            -5,  0,  5,  5,  5,  5,  0, -5,
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
            -10,-20,-20,-20,-20,-20,-20,-10,
            20, 20,  0,  0,  0,  0, 20, 20,
            20, 30, 10,  0,  0, 10, 30, 20
        }
    };

    const int PSQT_ENDGAME[8][64] = {
        {0},
        //PAWN
        {
            0,  0,  0,  0,  0,  0,  0,  0,
            50, 50, 50, 50, 50, 50, 50, 50, // 8...R7
            25, 15, 20, 15, 15, 20, 15, 25, //16...R6
            17, 15, 10, 2,  2, 10, 15,  17, //24...R5
            13, 7,  0, -2, -2,  0,  7,  13, //32...R4
            0,  0, -3,  2,  2, -3,  0,  0,  //40...R3
            0,  0,  10,-5, -5, 10,  0,  0,  //48...R2
            0,  0,  0,  0,  0,  0,  0,  0
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
            -20,-10, 30, 40, 40, 30,-10,-20,
            -20,-10, 30, 40, 40, 30,-10,-20,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-30,  0,  0,  0,  0,-30,-30,
            -50,-30,-20,-30,-30,-20,-30,-50
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
            8, 9, 10, 11, 12, 13, 14, 15,
            0, 1, 2, 3, 4, 5, 6, 7
        },
        //BLACK
        {
            7, 6, 5, 4, 3, 2, 1, 0,
            15, 14, 13, 12, 11, 10, 9, 8,
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
