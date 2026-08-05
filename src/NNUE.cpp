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
//   - SIMD: Uses AVX/SSE intrinsics for vectorized layer computation
//
// Efficiency: Only the first layer (largest) needs incremental updates.
// On most moves, we add/remove a few features instead of recomputing all the inputs.

#include "NNUE.h"
#include "BitboardUtils.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <memory>

#if defined(__AVX2__)
    #include <immintrin.h>
#endif

namespace {
   constexpr int KING_BUCKET_MULTIPLIER = 640;
   constexpr int PIECE_INDEX_MULTIPLIER = 64;
}

NNUE::NNUE() {
    m_isLoaded = false;
    m_filepath = "network-20260712.nnue";

    for(int ply = 0; ply < MAX_PLY_HISTORY; ply++) {
        for(int i = 0; i < NNUE_SIZE; i++) {
            m_accumulator[ply][0][i] = 0;
            m_accumulator[ply][1][i] = 0;
        }
    }
}

void NNUE::Load(std::string filepath) {
    if(!filepath.empty()) {
        m_filepath = filepath;
    }

    std::ifstream file;
    file.open(m_filepath.c_str(), std::ios::binary);

    if(!file.is_open()) {
        std::cout << "ERROR: NNUE file not found: " << m_filepath << std::endl;
        return;
    }

    auto nnue_storage = std::make_unique<NetworkStorage>();
    file.read((char*)nnue_storage.get(), sizeof(NetworkStorage));

    for(size_t i = 0; i < (sizeof(nnue_storage->w1) / sizeof(nnue_storage->w1[0])); i++) {
        m_network.w1[i] = (float)nnue_storage->w1[i] / NNUEConstants::CONVERSION_FACTOR;
    }

    size_t size = sizeof(nnue_storage->b1);
    size += sizeof(nnue_storage->w2) + sizeof(nnue_storage->b2);
    size += sizeof(nnue_storage->w3) + sizeof(nnue_storage->b3);
    size += sizeof(nnue_storage->w4) + sizeof(nnue_storage->b4);
    std::memcpy(m_network.b1, nnue_storage->b1, size);

    if(file.gcount()) {
        std::cout << "NNUE loaded: " << m_filepath << std::endl;
        m_isLoaded = true;
    } else {
        std::cout << "ERROR: NNUE not loaded correctly from file: " << m_filepath << std::endl;
    }

    file.close();
}

int NNUE::Evaluate(int color, int ply) const {
    //Layer 1
    alignas(32) float outputLayer1[NNUE_SIZE * 2];

    ActivateReLU(m_accumulator[ply][color], outputLayer1, NNUE_SIZE);
    ActivateReLU(m_accumulator[ply][1-color], outputLayer1 + NNUE_SIZE, NNUE_SIZE);

    //Layers 2,3,4
    alignas(32) float o2[ ARCH[L3][ROW] ];
    alignas(32) float o3[ ARCH[L4][ROW] ];
    float o4[1];

    ComputeLayer(outputLayer1, o2, m_network.b2, m_network.w2, ARCH[L2][ROW], ARCH[L2][COL], true);
    ComputeLayer(o2, o3, m_network.b3, m_network.w3, ARCH[L3][ROW], ARCH[L3][COL], true);
    ComputeLayer(o3, o4, m_network.b4, m_network.w4, ARCH[L4][ROW], ARCH[L4][COL], false);

    return CastInt(o4[0] * 100);
}

void NNUE::SetPieces(int color, uint64_t& pieces) {
    m_pieces[color] = &pieces;
}

void NNUE::Inputs_FullUpdate(int ply) {
    float* acc_w = m_accumulator[ply][0];
    float* acc_b = m_accumulator[ply][1];

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

    float* acc_w = m_accumulator[ply][0];
    float* acc_b = m_accumulator[ply][1];

    const float* weights_w = &m_network.w1[NNUE_SIZE * feature_w];
    const float* weights_b = &m_network.w1[NNUE_SIZE * feature_b];

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

    float* acc_w = m_accumulator[ply][0];
    float* acc_b = m_accumulator[ply][1];

    const float* weights_w = &m_network.w1[NNUE_SIZE * feature_w];
    const float* weights_b = &m_network.w1[NNUE_SIZE * feature_b];

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

    float* acc_w = m_accumulator[ply][0];
    float* acc_b = m_accumulator[ply][1];

    const float* weights_from_w = &m_network.w1[NNUE_SIZE * feature_from_w];
    const float* weights_from_b = &m_network.w1[NNUE_SIZE * feature_from_b];
    const float* weights_to_w = &m_network.w1[NNUE_SIZE * feature_to_w];
    const float* weights_to_b = &m_network.w1[NNUE_SIZE * feature_to_b];

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

// Clamps the input to the range [0,1]
void NNUE::ActivateReLU(const float* input, float* output, int size) const {
    #if defined(__AVX2__)
        __m256 zero = _mm256_setzero_ps();
        __m256 one = _mm256_set1_ps(1.0f);

        for(int i = 0; i < size; i += 8) {
            __m256 val = _mm256_load_ps(&input[i]);
            val = _mm256_max_ps(val, zero);
            val = _mm256_min_ps(val, one);
            _mm256_store_ps(&output[i], val);
        }
    #else
        for(int i = 0; i < size; i++) {
            output[i] = Clamp(input[i]);
        }
    #endif
}

namespace {
    #if defined(__AVX2__)
    //Horizontal sum of 8 floats (256-bits)
    inline float HorizontalSum256(__m256 v) {
        const __m128 r4 = _mm_add_ps( _mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1) );
        const __m128 r2 = _mm_add_ps( r4, _mm_movehl_ps( r4, r4 ) );
        const __m128 r1 = _mm_add_ss( r2, _mm_movehdup_ps( r2 ) );
        return _mm_cvtss_f32(r1);
    }
    #endif
}

void NNUE::ComputeLayer(const float* inputLayer, float* outputLayer,
                        const float* biases, const float* weights,
                        int dimInput, int dimOutput, bool with_ReLU) const
{
    for(int o = 0; o < dimOutput; o++) {
        float sum = biases[o];
        const int offset = o * dimInput;

        #if defined(__AVX2__)
            __m256 dot0 = _mm256_setzero_ps();
            __m256 dot1 = _mm256_setzero_ps();
            __m256 dot2 = _mm256_setzero_ps();
            __m256 dot3 = _mm256_setzero_ps();

            for(int i = 0; i < dimInput; i += 32) {
                __m256 inputs0 = _mm256_loadu_ps(&inputLayer[i +  0]);
                __m256 inputs1 = _mm256_loadu_ps(&inputLayer[i +  8]);
                __m256 inputs2 = _mm256_loadu_ps(&inputLayer[i + 16]);
                __m256 inputs3 = _mm256_loadu_ps(&inputLayer[i + 24]);

                __m256 weights0 = _mm256_loadu_ps(&weights[offset + i +  0]);
                __m256 weights1 = _mm256_loadu_ps(&weights[offset + i +  8]);
                __m256 weights2 = _mm256_loadu_ps(&weights[offset + i + 16]);
                __m256 weights3 = _mm256_loadu_ps(&weights[offset + i + 24]);

                dot0 = _mm256_fmadd_ps(inputs0, weights0, dot0);
                dot1 = _mm256_fmadd_ps(inputs1, weights1, dot1);
                dot2 = _mm256_fmadd_ps(inputs2, weights2, dot2);
                dot3 = _mm256_fmadd_ps(inputs3, weights3, dot3);
            }
            
            dot0 = _mm256_add_ps( dot0, dot1 );
            dot2 = _mm256_add_ps( dot2, dot3 );
            dot0 = _mm256_add_ps( dot0, dot2 );

            sum += HorizontalSum256(dot0);
        #else
            for(int i = 0; i < dimInput; i++) {
                sum += inputLayer[i] * weights[offset + i];
            }
        #endif

        if(with_ReLU) {
            outputLayer[o] = Clamp(sum);
        } else {
            outputLayer[o] = sum;
        }
    }
}
