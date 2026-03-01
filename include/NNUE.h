#pragma once

#include "Constants.h"

#include <memory>
#include <string>

const int NNUE_SIZE = 128;
const int NNUE_FEATURES = 32*64*5*2; //kingBuckets * square * pieceType * color
const int CONVERSION_FACTOR = INFINITE_I16 / 3;

struct Network;

class NNUE {
public:
    NNUE();
    static void Load(std::string filepath = "");
    static bool IsLoaded() { return m_isLoaded; }
    static std::string GetPath() { return m_filepath; }

    int Evaluate(int color) const;

    void SetPieces(int color, uint64_t& pieces);
    void SavePosition(int ply);
    void RestorePosition(int ply);

    void Inputs_FullUpdate();
    void Inputs_AddPiece(int color, int pieceType, int square);
    void Inputs_RemovePiece(int color, int pieceType, int square);
    void Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq);

private:
    //Helpers
    static float Clamp(float n);
    static void ComputeLayer(float* inputLayer, float* outputLayer, float* biases, float* weights, int dimInput, int dimOutput, bool with_ReLU);

    //Current state
    Bitboard* m_pieces[2];
    alignas(32) float m_accumulator[2][NNUE_SIZE];
    //Backups
    struct alignas(32) AccumulatorEntry { float v[2][NNUE_SIZE]; };
    std::unique_ptr<AccumulatorEntry[]> m_backupAccumulator;

    static bool m_isLoaded;
    static std::string m_filepath;
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
