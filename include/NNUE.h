#pragma once

#include "NNUE_Architecture.h"

#include <string>

struct SharedNetwork {
    // The actual network (from the binary file)
    Network network;

    // State of "Load"
    bool isLoaded = false;
    std::string filepath = "network-20260806.nnue";

    bool Load(const std::string& path);
};

class NNUE {
public:
    NNUE();

    int Evaluate(int color, int ply) const;

    void SetPieces(int color, u64& pieces);

    void Inputs_FullUpdate(int ply);
    void Inputs_AddPiece(int color, int pieceType, int square, int ply);
    void Inputs_RemovePiece(int color, int pieceType, int square, int ply);
    void Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq, int ply);

    void CopyAccumulator(int fromPly, int toPly);
    
    static bool Load(const std::string& path = "") { return s_shared.Load(path); }
    static bool IsLoaded() { return s_shared.isLoaded; }
    static std::string GetPath() { return s_shared.filepath; }

private:
    void ActivateReLU(const i16* input, i16* output, int size) const;

    template <typename T, bool with_ReLU>
    void ComputeLayer(const i16* inputLayer, T* outputLayer,
                      const i32* biases, const i16* weights,
                      int dimInput, int dimOutput) const;

private:
    inline static SharedNetwork s_shared;

    //Local state
    Bitboard* m_pieces[2];
    alignas(32) i16 m_accumulator[MAX_PLY_HISTORY][2][NNUE_SIZE]; // [PLY][COLOR][NNUE_SIZE]
};
