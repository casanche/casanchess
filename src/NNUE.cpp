// NNUE.cpp
//
// NNUE (Efficiently Updatable Neural Network) evaluation.
// A neural network that evaluates chess positions.
//
// Architecture: HalfKP with a linear feature bypass
//
// Input (features):
//   26 king buckets × 64 squares × 5 non-king piece types × 2 colors = 16,640 features
//
// Layers:
//   L1: 16,640 features → 256 accumulators (NNUE_SIZE x 2)
//   L2: 256 → 48 (NNUE_HIDDEN_SIZE)
//   L3: 48 → 1 (eval)
//   Bypass: 16,640 features → 1 (eval)
//   Drawishness: 256 → 1 signed draw residual (post-training)
//
// Accumulators are updated incrementally when pieces move (basically the point of NNUE).
// AVX2 instrinsics are used for fast vectorized layer computation.

#include "NNUE.h"
#include "BitboardUtils.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

#if defined(__AVX2__)
    #include <immintrin.h>
#endif

namespace {
    constexpr int KING_BUCKET_MULTIPLIER = 640;
    constexpr int PIECE_INDEX_MULTIPLIER = 64;

    constexpr i32 SCReLU(i32 value) {
        value = std::clamp(value, 0, NNUEConstants::QUANT_FACTOR_L1);
        return (value * value + NNUEConstants::QUANT_FACTOR_L1 / 2)
                    / NNUEConstants::QUANT_FACTOR_L1;
    }

    #if defined(__AVX2__)
    constexpr int SIMD_WIDTH = 16;

    inline i32 HorizontalSum128(__m128i x) {
        x = _mm_add_epi32(x, _mm_srli_si128(x, 8));
        x = _mm_add_epi32(x, _mm_srli_si128(x, 4));
        return _mm_cvtsi128_si32(x);
    }
    #endif

    inline i32 DotProduct(const i16* values, const i16* weights, int size) {
        i32 sum = 0;
        int i = 0;

        #if defined(__AVX2__)
            __m256i dot = _mm256_setzero_si256();

            for(; i + SIMD_WIDTH <= size; i += SIMD_WIDTH) {
                const __m256i valuesVec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(values + i));
                const __m256i weightsVec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(weights + i));

                dot = _mm256_add_epi32(dot, _mm256_madd_epi16(valuesVec, weightsVec));
            }

            const __m128i halves = _mm_add_epi32(
                _mm256_castsi256_si128(dot),
                _mm256_extracti128_si256(dot, 1)
            );
            sum = HorizontalSum128(halves);
        #endif

        for(; i < size; i++)
            sum += values[i] * weights[i];

        return sum;
    }
}

// =================
// ===== Load ======
// =================

bool NNUE::LoadBytes(std::span<const std::byte> bytes) {
    if(bytes.size() != sizeof(Network))
        return false;

    std::memcpy(&s_network, bytes.data(), sizeof(Network));
    return true;
}

bool NNUE::LoadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if(!file) {
        std::cerr << "info string ERROR: NNUE file not found: " << path << std::endl;
        return false;
    }

    if(file.tellg() != static_cast<std::streamoff>(sizeof(Network))) {
        std::cerr << "info string ERROR: NNUE file size mismatch: " << path << std::endl;
        return false;
    }

    auto candidate = std::make_unique_for_overwrite<Network>();

    file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(candidate.get()), sizeof(Network))) {
        std::cerr << "info string ERROR: Could not read NNUE file: " << path << std::endl;
        return false;
    }

    std::memcpy(&s_network, candidate.get(), sizeof(Network));

    std::cout << "info string NNUE loaded: " << path << std::endl;
    return true;
}

// ================
// ===== NNUE =====
// ================

NNUE::NNUE() {
    std::memset(m_state->accumulator, 0, sizeof(m_state->accumulator));
    std::memset(m_state->linearAccumulator, 0, sizeof(m_state->linearAccumulator));
}

// Deep copy
NNUE::NNUE(const NNUE& other) {
    std::memcpy(m_state->accumulator, other.m_state->accumulator, sizeof(m_state->accumulator));
    std::memcpy(m_state->linearAccumulator, other.m_state->linearAccumulator, sizeof(m_state->linearAccumulator));
}

// Deep assignment
NNUE& NNUE::operator=(const NNUE& other) {
    if(this != &other) {
        if(!m_state)
            m_state = std::make_unique<NNUE_State>();
        std::memcpy(m_state->accumulator, other.m_state->accumulator, sizeof(m_state->accumulator));
        std::memcpy(m_state->linearAccumulator, other.m_state->linearAccumulator, sizeof(m_state->linearAccumulator));
    }
    return *this;
}

int NNUE::Evaluate(int color, int ply) const {
    i16 o1[NNUE_SIZE * 2]; //Layer 1 activated accumulator
    ActivateSCReLU(m_state->accumulator[ply][color], o1);
    ActivateSCReLU(m_state->accumulator[ply][1-color], o1 + NNUE_SIZE);

    return EvaluateFromActivated(o1, color, ply);
}

EvaluationOutput NNUE::EvaluateOutputs(int color, int ply) const {
    i16 o1[NNUE_SIZE * 2];
    ActivateSCReLU(m_state->accumulator[ply][color], o1);
    ActivateSCReLU(m_state->accumulator[ply][1-color], o1 + NNUE_SIZE);

    return {
        EvaluateFromActivated(o1, color, ply),
        DrawishnessFromActivated(o1)
    };
}

int NNUE::EvaluateFromActivated(const i16* activated, int color, int ply) const {
    i16 hidden[ ARCH[L2][COL] ];

    ComputeActivatedLayer(activated, hidden, s_network.b2, s_network.w2, ARCH[L2][ROW], ARCH[L2][COL]);

    i32 nonlinear = s_network.b3[0];
    nonlinear += DotProduct(hidden, s_network.w3, ARCH[L3][ROW]);

    i32 linear = m_state->linearAccumulator[ply][color];
    linear -= m_state->linearAccumulator[ply][1-color];

    const i32 output = nonlinear + linear * NNUEConstants::QUANT_FACTOR_W;

    constexpr int EVAL_K = 100;
    return output * EVAL_K / NNUEConstants::QUANT_FACTOR_B;
}

int NNUE::DrawishnessFromActivated(const i16* activated) const {
    i64 residual = s_network.drawB;
    residual += DotProduct(activated, s_network.drawW, ARCH[L2][ROW]);

    return static_cast<int>(residual * 100 / NNUEConstants::QUANT_FACTOR_B);
}

void NNUE::Inputs_FullUpdate(int ply, const PieceBitboards pieces) {
    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] = s_network.b1[i];
        acc_b[i] = s_network.b1[i];
    }
    m_state->linearAccumulator[ply][WHITE] = 0;
    m_state->linearAccumulator[ply][BLACK] = 0;

    int kingSquare_w = BitscanForward(pieces[WHITE][KING]);
    int kingSquare_b = BitscanForward(pieces[BLACK][KING]);

    for(int color = WHITE; color <= BLACK; color++) {
        for(int pieceType = PAWN; pieceType <= QUEEN; pieceType++) {
            Bitboard bitboard = pieces[color][pieceType];
            for(int square : BitboardIterator(bitboard)) {
                Inputs_AddPiece(color, pieceType-1, square, ply, kingSquare_w, kingSquare_b);
            }
        }
    }
}

namespace {
    constexpr int GetFeatureIndex(int color, int pieceType, int square, int kingSquare) {
        const int kingBucket = NNUEConstants::KING_BUCKETS[kingSquare];
        const int index = (pieceType * 2) + (color);
    
        return (KING_BUCKET_MULTIPLIER * kingBucket) + (PIECE_INDEX_MULTIPLIER * index) + square;
    }
}

void NNUE::Inputs_AddPiece(int color, int pieceType, int square, int ply, int kingSquare_w, int kingSquare_b) {
    kingSquare_b ^= NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int square_w = square;
    const int square_b = square ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_w = GetFeatureIndex(color,   pieceType, square_w, kingSquare_w);
    const int feature_b = GetFeatureIndex(1-color, pieceType, square_b, kingSquare_b);

    assert(feature_w < NNUE_FEATURES);
    assert(feature_b < NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_w = &s_network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &s_network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] += weights_w[i];
        acc_b[i] += weights_b[i];
    }
    m_state->linearAccumulator[ply][WHITE] += s_network.linearW[feature_w];
    m_state->linearAccumulator[ply][BLACK] += s_network.linearW[feature_b];
}

void NNUE::Inputs_RemovePiece(int color, int pieceType, int square, int ply, int kingSquare_w, int kingSquare_b) {
    kingSquare_b ^= NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int square_w = square;
    const int square_b = square ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_w = GetFeatureIndex(color,   pieceType, square_w, kingSquare_w);
    const int feature_b = GetFeatureIndex(1-color, pieceType, square_b, kingSquare_b);

    assert(feature_w < NNUE_FEATURES);
    assert(feature_b < NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_w = &s_network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &s_network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_w[i];
        acc_b[i] -= weights_b[i];
    }
    m_state->linearAccumulator[ply][WHITE] -= s_network.linearW[feature_w];
    m_state->linearAccumulator[ply][BLACK] -= s_network.linearW[feature_b];
}

void NNUE::Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq, int ply, int kingSquare_w, int kingSquare_b) {
    kingSquare_b ^= NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int fromSq_w = fromSq;
    const int fromSq_b = fromSq ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int toSq_w = toSq;
    const int toSq_b = toSq ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_from_w = GetFeatureIndex(color,   pieceType, fromSq_w, kingSquare_w);
    const int feature_from_b = GetFeatureIndex(1-color, pieceType, fromSq_b, kingSquare_b);

    const int feature_to_w = GetFeatureIndex(color,   pieceType, toSq_w, kingSquare_w);
    const int feature_to_b = GetFeatureIndex(1-color, pieceType, toSq_b, kingSquare_b);

    assert(feature_from_w < NNUE_FEATURES);
    assert(feature_from_b < NNUE_FEATURES);

    assert(feature_to_w < NNUE_FEATURES);
    assert(feature_to_b < NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_from_w = &s_network.w1[NNUE_SIZE * feature_from_w];
    const i16* weights_from_b = &s_network.w1[NNUE_SIZE * feature_from_b];
    const i16* weights_to_w = &s_network.w1[NNUE_SIZE * feature_to_w];
    const i16* weights_to_b = &s_network.w1[NNUE_SIZE * feature_to_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_from_w[i];
        acc_b[i] -= weights_from_b[i];

        acc_w[i] += weights_to_w[i];
        acc_b[i] += weights_to_b[i];

    }
    m_state->linearAccumulator[ply][WHITE] -= s_network.linearW[feature_from_w];
    m_state->linearAccumulator[ply][BLACK] -= s_network.linearW[feature_from_b];
    m_state->linearAccumulator[ply][WHITE] += s_network.linearW[feature_to_w];
    m_state->linearAccumulator[ply][BLACK] += s_network.linearW[feature_to_b];
}

void NNUE::CopyAccumulator(int fromPly, int toPly) {
    std::memcpy(&m_state->accumulator[toPly], m_state->accumulator[fromPly], sizeof(m_state->accumulator[0]));
    std::memcpy(&m_state->linearAccumulator[toPly], m_state->linearAccumulator[fromPly], sizeof(m_state->linearAccumulator[0]));
}

void NNUE::ActivateSCReLU(const i16* input, i16* output) const {
    #if defined(__AVX2__)
        constexpr int SCRELU_SHIFT = 8;
        static_assert(NNUEConstants::QUANT_FACTOR_L1 == (1 << SCRELU_SHIFT));

        const __m256i zero = _mm256_setzero_si256();
        const __m256i max = _mm256_set1_epi16(NNUEConstants::QUANT_FACTOR_L1);
        const __m256i rounding = _mm256_set1_epi16(NNUEConstants::QUANT_FACTOR_L1 / 2);

        for(int i = 0; i < NNUE_SIZE; i += SIMD_WIDTH) {
            __m256i value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));
            value = _mm256_max_epi16(value, zero);
            value = _mm256_min_epi16(value, max);

            // 256 squared overflows 16 bits, so restore its result afterwards
            const __m256i atMaximum = _mm256_cmpeq_epi16(value, max);

            value = _mm256_mullo_epi16(value, value);
            value = _mm256_add_epi16(value, rounding);
            value = _mm256_srli_epi16(value, SCRELU_SHIFT);
            value = _mm256_blendv_epi8(value, max, atMaximum);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), value);
        }
    #else
        for(int i = 0; i < NNUE_SIZE; i++)
            output[i] = static_cast<i16>(SCReLU(input[i]));
    #endif
}

void NNUE::ComputeActivatedLayer(const i16* inputLayer, i16* outputLayer, const i32* biases,
                                 const i16* weights, int dimInput, int dimOutput) const
{
    for(int o = 0; o < dimOutput; o++) {
        const int offset = o * dimInput;
        i32 sum = biases[o] + DotProduct(inputLayer, weights + offset, dimInput);

        sum /= NNUEConstants::QUANT_FACTOR_W; // Revert scaling
        outputLayer[o] = static_cast<i16>(SCReLU(sum));
    }
}
