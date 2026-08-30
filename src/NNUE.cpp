// NNUE.cpp
//
// NNUE (Efficiently Updatable Neural Network) evaluation.
// A neural network that evaluates chess positions.
//
// Optimized for incremental updates of the first layer (basically the point of NNUE).
//
// Architecture: HalfKP with linear bypass and Drawishness head
//   Features: 26 king buckets × 64 squares × 5 piece types × 2 colors = 16640 features
//   Layers: (NNUE_SIZE x 2) → NNUE_HIDDEN_SIZE → 1 + linear feature bypass
//      L1: White and black accumulators feed the nonlinear branch
//      Linear: Separate scalar accumulator updated from the same active features
//      L2: Hidden layer, to account for non-linearities
//      L3 (1): Output node added to the linear us-them score
//      Drawishness (1): x_clamp256 residual, evaluated on demand
//
// Key concepts:
//   - King buckets: 26 partitions of king position for learning king-relative patterns.
//   - Accumulator: Cached first layer output, updated incrementally on piece moves.
//   - Perspective: Each side has its own accumulator (us/them perspective).
//   - SIMD: Use AVX2 intrinsics for fast vectorized layer computation.

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
}

// =========================
// ===== SharedNetwork =====
// =========================

bool SharedNetwork::Load(const std::string& path) {
    if(!path.empty())
        filepath = path;

    std::ifstream file(filepath, std::ios::binary);

    if(!file.is_open()) {
        std::cerr << "info string ERROR: NNUE file not found: " << filepath << std::endl;
        isLoaded = false;
        return false;
    }

    file.read(reinterpret_cast<char*>(&network), sizeof(Network));
    const bool exactSize = file.gcount() == sizeof(Network)
                        && file.peek() == std::ifstream::traits_type::eof();

    if(exactSize) {
        std::cout << "info string NNUE loaded: " << filepath << std::endl;
        isLoaded = true;
    } else {
        std::cout << "info string ERROR: NNUE file size mismatch or corrupted: " << filepath << std::endl;
        isLoaded = false;
    }

    return isLoaded;
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
    i16 o1[NNUE_SIZE * 2]; //Layer 1 (ReLU'ed accumulator)

    ActivateReLU(m_state->accumulator[ply][color], o1, NNUE_SIZE);
    ActivateReLU(m_state->accumulator[ply][1-color], o1 + NNUE_SIZE, NNUE_SIZE);

    i16 o2[ ARCH[L2][COL] ]; //Layer 2
    i32 o3[ ARCH[L3][COL] ]; //Layer 3

    ComputeLayer<i16, true>(o1, o2, s_shared.network.b2, s_shared.network.w2, ARCH[L2][ROW], ARCH[L2][COL]);
    ComputeLayer<i32, false>(o2, o3, s_shared.network.b3, s_shared.network.w3, ARCH[L3][ROW], ARCH[L3][COL]);

    const i32 linear = m_state->linearAccumulator[ply][color]
                     - m_state->linearAccumulator[ply][1-color];

    return (o3[0] + linear * NNUEConstants::QUANT_FACTOR_W) * 100 / NNUEConstants::QUANT_FACTOR_B;
}

int NNUE::Drawishness(int color, int ply) const {
    i16 o1[NNUE_SIZE * 2];
    ActivateReLU(m_state->accumulator[ply][color], o1, NNUE_SIZE);
    ActivateReLU(m_state->accumulator[ply][1-color], o1 + NNUE_SIZE, NNUE_SIZE);

    i64 residual = s_shared.network.drawB;
    for(uint i = 0; i < ARCH[L2][ROW]; i++)
        residual += static_cast<i64>(o1[i]) * s_shared.network.drawW[i];

    return static_cast<int>(residual * 100 / NNUEConstants::QUANT_FACTOR_B);
}

void NNUE::Inputs_FullUpdate(int ply, const PieceBitboards pieces) {
    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] = s_shared.network.b1[i];
        acc_b[i] = s_shared.network.b1[i];
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

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_w = &s_shared.network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &s_shared.network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] += weights_w[i];
        acc_b[i] += weights_b[i];
    }
    m_state->linearAccumulator[ply][WHITE] += s_shared.network.linearW[feature_w];
    m_state->linearAccumulator[ply][BLACK] += s_shared.network.linearW[feature_b];
}

void NNUE::Inputs_RemovePiece(int color, int pieceType, int square, int ply, int kingSquare_w, int kingSquare_b) {
    kingSquare_b ^= NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int square_w = square;
    const int square_b = square ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_w = GetFeatureIndex(color,   pieceType, square_w, kingSquare_w);
    const int feature_b = GetFeatureIndex(1-color, pieceType, square_b, kingSquare_b);

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_w = &s_shared.network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &s_shared.network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_w[i];
        acc_b[i] -= weights_b[i];
    }
    m_state->linearAccumulator[ply][WHITE] -= s_shared.network.linearW[feature_w];
    m_state->linearAccumulator[ply][BLACK] -= s_shared.network.linearW[feature_b];
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

    assert(feature_from_w <= NNUE_FEATURES);
    assert(feature_from_b <= NNUE_FEATURES);

    assert(feature_to_w <= NNUE_FEATURES);
    assert(feature_to_b <= NNUE_FEATURES);

    i16* acc_w = m_state->accumulator[ply][0];
    i16* acc_b = m_state->accumulator[ply][1];

    const i16* weights_from_w = &s_shared.network.w1[NNUE_SIZE * feature_from_w];
    const i16* weights_from_b = &s_shared.network.w1[NNUE_SIZE * feature_from_b];
    const i16* weights_to_w = &s_shared.network.w1[NNUE_SIZE * feature_to_w];
    const i16* weights_to_b = &s_shared.network.w1[NNUE_SIZE * feature_to_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_from_w[i];
        acc_b[i] -= weights_from_b[i];

        acc_w[i] += weights_to_w[i];
        acc_b[i] += weights_to_b[i];

    }
    m_state->linearAccumulator[ply][WHITE] -= s_shared.network.linearW[feature_from_w];
    m_state->linearAccumulator[ply][BLACK] -= s_shared.network.linearW[feature_from_b];
    m_state->linearAccumulator[ply][WHITE] += s_shared.network.linearW[feature_to_w];
    m_state->linearAccumulator[ply][BLACK] += s_shared.network.linearW[feature_to_b];
}

void NNUE::CopyAccumulator(int fromPly, int toPly) {
    std::memcpy(&m_state->accumulator[toPly], m_state->accumulator[fromPly], sizeof(m_state->accumulator[0]));
    std::memcpy(&m_state->linearAccumulator[toPly], m_state->linearAccumulator[fromPly], sizeof(m_state->linearAccumulator[0]));
}

// Clamps the input to the range [0,255]
void NNUE::ActivateReLU(const i16* input, i16* output, int size) const {
    #if defined(__AVX2__)
        __m256i zero = _mm256_setzero_si256();
        __m256i max = _mm256_set1_epi16(255);

        for(int i = 0; i < size; i += 16) {
            __m256i val = _mm256_loadu_si256((__m256i*)&input[i]);
            val = _mm256_max_epi16(val, zero);
            val = _mm256_min_epi16(val, max);

            _mm256_storeu_si256((__m256i*)&output[i], val);
        }
    #else
        for(int i = 0; i < size; i++) {
            output[i] = std::clamp(input[i], (i16)0, (i16)255);
        }
    #endif
}

namespace {
    #if defined(__AVX2__)
    // Horizontal sum of 4 integers (128-bits)
    inline i32 HorizontalSum128(__m128i x) {
        x = _mm_add_epi32(x, _mm_srli_si128(x, 8));
        x = _mm_add_epi32(x, _mm_srli_si128(x, 4));
        return _mm_cvtsi128_si32(x);
    }
    #endif
}

template <typename T, bool with_ReLU>
void NNUE::ComputeLayer(const i16* inputLayer, T* outputLayer,
                        const i32* biases, const i16* weights,
                        int dimInput, int dimOutput) const
{
    for(int o = 0; o < dimOutput; o++) {
        i32 sum = biases[o];
        const int offset = o * dimInput;

        #if defined(__AVX2__)
            __m256i dot = _mm256_setzero_si256();

            for(int i = 0; i < dimInput; i += 16) {
                __m256i inputVec = _mm256_loadu_si256((__m256i*)&inputLayer[i]);
                __m256i weightsVec = _mm256_loadu_si256((__m256i*)&weights[offset + i]);

                __m256i product = _mm256_madd_epi16(inputVec, weightsVec);
                dot = _mm256_add_epi32(dot, product);
            }

            __m128i x = _mm_add_epi32(_mm256_castsi256_si128(dot), _mm256_extracti128_si256(dot, 1));

            sum += HorizontalSum128(x);
        #else
            for(int i = 0; i < dimInput; i++) {
                sum += inputLayer[i] * weights[offset + i];
            }
        #endif

        if constexpr (with_ReLU) {
            sum /= NNUEConstants::QUANT_FACTOR_W; // Revert scaling
            outputLayer[o] = static_cast<T>(std::clamp(sum, 0, 255));
        } else {
            outputLayer[o] = static_cast<T>(sum);
        }
    }
}

// The compiler needs to know which types to generate in since the template is defined in .cpp
template void NNUE::ComputeLayer<i16, true>(const i16*, i16*, const i32*, const i16*, int, int) const;
template void NNUE::ComputeLayer<i32, false>(const i16*, i32*, const i32*, const i16*, int, int) const;
