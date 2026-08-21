#pragma once

#include "NNUE_Architecture.h"

#include <string>
#include <memory>

using PieceBitboards = Bitboard[2][8]; //[COLOR][PIECE_TYPE]

struct SharedNetwork {
    // The actual network (from the binary file)
    Network network;

    // State of "Load"
    bool isLoaded = false;
    std::string filepath = "network-20260806.nnue";

    bool Load(const std::string& path);
};

struct NNUE_State {
    alignas(32) i16 accumulator[MAX_PLY_HISTORY][2][NNUE_SIZE]; // [PLY][COLOR][NNUE_SIZE]
};

class NNUE {
public:
    NNUE();

    NNUE(const NNUE& other);
    NNUE& operator=(const NNUE& other);
    ~NNUE() = default;

    int Evaluate(int color, int ply) const;

    void Inputs_FullUpdate(int ply, const PieceBitboards pieces);
    void Inputs_AddPiece(int color, int pieceType, int square, int ply, int kingSquare_w, int kingSquare_b);
    void Inputs_RemovePiece(int color, int pieceType, int square, int ply, int kingSquare_w, int kingSquare_b);
    void Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq, int ply, int kingSquare_w, int kingSquare_b);

    void CopyAccumulator(int fromPly, int toPly);
    
    static bool Load(const std::string& path = "") { return s_shared.Load(path); }
    static bool IsLoaded() { return s_shared.isLoaded; }
    static std::string GetPath() { return s_shared.filepath; }

private:
    void ActivateReLU(const i16* input, i16* output, int size) const;

    i32 ComputeBypass(const i16* input, const i16* weights) const;

    template <typename T, bool with_ReLU>
    void ComputeLayer(const i16* inputLayer, T* outputLayer,
                      const i32* biases, const i16* weights,
                      int dimInput, int dimOutput) const;

private:
    // Global
    inline static SharedNetwork s_shared;

    // Local
    std::unique_ptr<NNUE_State> m_state = std::make_unique<NNUE_State>();
};
