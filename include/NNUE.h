#pragma once

#include "Constants.h"

#include <string>

constexpr int NNUE_SIZE = 128;
constexpr int NNUE_FEATURES = 32*64*5*2; //kingBuckets * square * pieceType * color

namespace NNUEConstants {
    constexpr int BLACK_PERSPECTIVE_XOR = 56;
    constexpr int CONVERSION_FACTOR = INFINITE_I16 / 3;
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
}

struct Network;

class NNUE {
public:
    NNUE();
    void Load(std::string filepath = "");
    bool IsLoaded() const { return m_isLoaded; }
    std::string GetPath() const { return m_filepath; }

    int Evaluate(int color, int ply);

    void SetPieces(int color, uint64_t& pieces);

    void Inputs_FullUpdate(int ply);
    void Inputs_AddPiece(int color, int pieceType, int square, int ply);
    void Inputs_RemovePiece(int color, int pieceType, int square, int ply);
    void Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq, int ply);

    void CopyAccumulator(int fromPly, int toPly);

private:
    //Helpers
    float Clamp(float n);

    //Compute
    void ComputeLayer(const float* inputLayer, float* outputLayer,
                      const float* biases, const float* weights,
                      int dimInput, int dimOutput, bool with_ReLU);

    //Current state
    Bitboard* m_pieces[2];

    bool m_isLoaded;
    std::string m_filepath;
    
    alignas(32) float m_accumulator[MAX_PLY_HISTORY][2][NNUE_SIZE]; // [PLY][COLOR][NNUE_SIZE]
};

//Network architecture
enum NNUE_LAYER { L1, L2, L3, L4, NNUE_LAYERS };
enum PARAMETER_TYPE { W, B, PARAMETER_TYPES };
enum DIMENSIONS { ROW, COL, DIMENSIONS };

constexpr uint ARCH[NNUE_LAYERS][DIMENSIONS] = {
    {NNUE_FEATURES, NNUE_SIZE}, //Layer1
    {2*NNUE_SIZE, 32},          //Layer2
    {32, 32},                   //Layer3
    {32, 1}                     //Layer4
};
constexpr uint ARCH_DIMENSIONS[NNUE_LAYERS][PARAMETER_TYPES] = {
    {ARCH[L1][ROW] * ARCH[L1][COL], ARCH[L1][COL]},
    {ARCH[L2][ROW] * ARCH[L2][COL], ARCH[L2][COL]},
    {ARCH[L3][ROW] * ARCH[L3][COL], ARCH[L3][COL]},
    {ARCH[L4][ROW] * ARCH[L4][COL], ARCH[L4][COL]},
};

struct alignas(32) Network {
    float w1[ ARCH_DIMENSIONS[L1][W] ];
    float b1[ ARCH_DIMENSIONS[L1][B] ];
    float w2[ ARCH_DIMENSIONS[L2][W] ];
    float b2[ ARCH_DIMENSIONS[L2][B] ];
    float w3[ ARCH_DIMENSIONS[L3][W] ];
    float b3[ ARCH_DIMENSIONS[L3][B] ];
    float w4[ ARCH_DIMENSIONS[L4][W] ];
    float b4[ ARCH_DIMENSIONS[L4][B] ];
};
struct NetworkStorage {
    int16_t w1[ ARCH_DIMENSIONS[L1][0] ];
    float b1[ ARCH_DIMENSIONS[L1][1] ];
    float w2[ ARCH_DIMENSIONS[L2][0] ];
    float b2[ ARCH_DIMENSIONS[L2][1] ];
    float w3[ ARCH_DIMENSIONS[L3][0] ];
    float b3[ ARCH_DIMENSIONS[L3][1] ];
    float w4[ ARCH_DIMENSIONS[L4][0] ];
    float b4[ ARCH_DIMENSIONS[L4][1] ];
};

inline Network m_network;
inline NNUE nnue;
