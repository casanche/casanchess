// NNUE.cpp
//
// NNUE (Efficiently Updatable Neural Network) evaluation.
// A neural network that evaluates chess positions.
//
// Optimized for incremental updates of the first layer (basically the point of NNUE).
//
// Architecture: HalfKP
//   Features: 32 king buckets × 64 squares × 5 piece types × 2 colors = 20480 features
//   Layers: (128x2) → 32 → 32 → 1
//      L1 (128x2): White and black accumulatores are concatenated
//      L2, L3: 2 hidden layers of 32 neurons each, to account for non-linearities
//      L4: 1 output neuron, representing the evaluation score in centipawns
//
// Key concepts:
//   - King buckets: 32 partitions of king position for learning king-relative patterns
//   - Accumulator: Cached first layer output, updated incrementally on piece moves
//   - Perspective: Each side has its own accumulator (white/black view of the board)
//   - SIMD: Uses AVX2 intrinsics for vectorized layer computation (integer quantization)
//
// Efficiency: Only the first layer (largest) needs incremental updates.
// On most moves, we add/remove a few features instead of recomputing all the inputs.

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

NNUE::NNUE() {
    m_isLoaded = false;
    m_filepath = "network-20260806.nnue";

    std::memset(m_accumulator, 0, sizeof(m_accumulator));
}

bool NNUE::Load(std::string filepath) {
    if(!filepath.empty()) {
        m_filepath = filepath;
    }

    std::ifstream file(m_filepath, std::ios::binary);

    if(!file.is_open()) {
        std::cerr << "info string ERROR: NNUE file not found: " << m_filepath << std::endl;
        m_isLoaded = false;
        return false;
    }

    file.read(reinterpret_cast<char*>(&m_network), sizeof(Network));

    if(file.gcount() == sizeof(Network)) {
        std::cout << "info string NNUE loaded: " << m_filepath << std::endl;
        m_isLoaded = true;
    } else {
        std::cout << "info string ERROR: NNUE file size mismatch or corrupted: " << m_filepath << std::endl;
        m_isLoaded = false;
    }

    return m_isLoaded;
}

int NNUE::Evaluate(int color, int ply) const {
    //Layer 1
    alignas(32) i16 outputLayer1[NNUE_SIZE * 2];

    ActivateReLU(m_accumulator[ply][color], outputLayer1, NNUE_SIZE);
    ActivateReLU(m_accumulator[ply][1-color], outputLayer1 + NNUE_SIZE, NNUE_SIZE);

    //Layers 2,3,4
    alignas(32) i16 o2[ ARCH[L3][ROW] ];
    alignas(32) i16 o3[ ARCH[L4][ROW] ];
    i32 o4[1];

    ComputeLayer<i16, true>(outputLayer1, o2, m_network.b2, m_network.w2, ARCH[L2][ROW], ARCH[L2][COL]);
    ComputeLayer<i16, true>(o2, o3, m_network.b3, m_network.w3, ARCH[L3][ROW], ARCH[L3][COL]);
    ComputeLayer<i32, false>(o3, o4, m_network.b4, m_network.w4, ARCH[L4][ROW], ARCH[L4][COL]);

    return (o4[0] * 100) / NNUEConstants::QUANT_FACTOR_B;
}

void NNUE::SetPieces(int color, u64& pieces) {
    m_pieces[color] = &pieces;
}

void NNUE::Inputs_FullUpdate(int ply) {
    i16* acc_w = m_accumulator[ply][0];
    i16* acc_b = m_accumulator[ply][1];

    for(int i=0; i < NNUE_SIZE; i++) {
        acc_w[i] = m_network.b1[i];
        acc_b[i] = m_network.b1[i];
    }

    for(int color = WHITE; color <= BLACK; color++) {
        for(int pieceType = PAWN; pieceType <= QUEEN; pieceType++) {
            Bitboard bitboard = m_pieces[color][pieceType];
            for(int square : BitboardIterator(bitboard)) {
                Inputs_AddPiece(color, pieceType-1, square, ply);
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

void NNUE::Inputs_AddPiece(int color, int pieceType, int square, int ply) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int square_w = square;
    const int square_b = square ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_w = GetFeatureIndex(color,   pieceType, square_w, kingSquare_w);
    const int feature_b = GetFeatureIndex(1-color, pieceType, square_b, kingSquare_b);

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    i16* acc_w = m_accumulator[ply][0];
    i16* acc_b = m_accumulator[ply][1];

    const i16* weights_w = &m_network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &m_network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] += weights_w[i];
        acc_b[i] += weights_b[i];
    }
}

void NNUE::Inputs_RemovePiece(int color, int pieceType, int square, int ply) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int square_w = square;
	const int square_b = square ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

    const int feature_w = GetFeatureIndex(color,   pieceType, square_w, kingSquare_w);
    const int feature_b = GetFeatureIndex(1-color, pieceType, square_b, kingSquare_b);

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    i16* acc_w = m_accumulator[ply][0];
    i16* acc_b = m_accumulator[ply][1];

    const i16* weights_w = &m_network.w1[NNUE_SIZE * feature_w];
    const i16* weights_b = &m_network.w1[NNUE_SIZE * feature_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_w[i];
        acc_b[i] -= weights_b[i];
    }
}

void NNUE::Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq, int ply) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ NNUEConstants::BLACK_PERSPECTIVE_XOR;

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

    i16* acc_w = m_accumulator[ply][0];
    i16* acc_b = m_accumulator[ply][1];

    const i16* weights_from_w = &m_network.w1[NNUE_SIZE * feature_from_w];
    const i16* weights_from_b = &m_network.w1[NNUE_SIZE * feature_from_b];
    const i16* weights_to_w = &m_network.w1[NNUE_SIZE * feature_to_w];
    const i16* weights_to_b = &m_network.w1[NNUE_SIZE * feature_to_b];

    for(int i = 0; i < NNUE_SIZE; i++) {
        acc_w[i] -= weights_from_w[i];
        acc_b[i] -= weights_from_b[i];

        acc_w[i] += weights_to_w[i];
        acc_b[i] += weights_to_b[i];

    }
}

void NNUE::CopyAccumulator(int fromPly, int toPly) {
    std::memcpy(&m_accumulator[toPly], m_accumulator[fromPly], sizeof(m_accumulator[0]));
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
