#pragma once

#include "Constants.h"

constexpr int NNUE_SIZE = 128;
constexpr int NNUE_HIDDEN_SIZE = 48;
constexpr int NNUE_FEATURES = 32*64*5*2; //kingBuckets * square * pieceType * color

namespace NNUEConstants {
    constexpr int BLACK_PERSPECTIVE_XOR = 56;
    constexpr u8 KING_BUCKETS[64] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9,10,11,12,13,14,15,
       16,16,17,17,18,18,19,19,
       20,20,21,21,22,22,23,23,
       24,24,25,25,26,26,27,27,
       24,24,25,25,26,26,27,27,
       28,28,29,29,30,30,31,31,
       28,28,29,29,30,30,31,31
   };
   // Quantization scale factors to convert weights to integers
   constexpr int QUANT_FACTOR_L1 = 256; // W1, B1
   constexpr int QUANT_FACTOR_W = 64; // W2, W3
   constexpr int QUANT_FACTOR_B = QUANT_FACTOR_L1 * QUANT_FACTOR_W; // B2, B3
}

// Network architecture (V1.4: 128 accumulators, 48 hidden neurons, Drawishness head)
enum NNUE_LAYER { L1, L2, L3, NNUE_LAYERS };
enum PARAMETER_TYPE { W, B, PARAMETER_TYPES };
enum DIMENSIONS { ROW, COL, DIMENSIONS };

constexpr uint ARCH[NNUE_LAYERS][DIMENSIONS] = {
    {NNUE_FEATURES, NNUE_SIZE},        //Layer1
    {2*NNUE_SIZE, NNUE_HIDDEN_SIZE},   //Layer2
    {NNUE_HIDDEN_SIZE, 1}              //Layer3
};

constexpr uint ARCH_DIMENSIONS[NNUE_LAYERS][PARAMETER_TYPES] = {
    {ARCH[L1][ROW] * ARCH[L1][COL], ARCH[L1][COL]},
    {ARCH[L2][ROW] * ARCH[L2][COL], ARCH[L2][COL]},
    {ARCH[L3][ROW] * ARCH[L3][COL], ARCH[L3][COL]},
};

struct alignas(32) Network {
    i16 w1[ ARCH_DIMENSIONS[L1][0] ];
    i16 b1[ ARCH_DIMENSIONS[L1][1] ];
    i16 linearW[ NNUE_FEATURES ];

    i16 w2[ ARCH_DIMENSIONS[L2][0] ];
    i32 b2[ ARCH_DIMENSIONS[L2][1] ];

    i16 w3[ ARCH_DIMENSIONS[L3][0] ];
    i32 b3[ ARCH_DIMENSIONS[L3][1] ];

    i16 drawW[ ARCH[L2][ROW] ];
    i32 drawB;
};
