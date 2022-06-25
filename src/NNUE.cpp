#include "NNUE.h"
#include "BitboardUtils.h"
#include <immintrin.h>
#include <cassert>
#include <cstring>
#include <fstream>

const u8 KING_BUCKETS[64] = {
	 0, 1, 2, 3, 4, 5, 6, 7,
	 8, 9,10,11,12,13,14,15,
	16,16,17,17,18,18,19,19,
	20,20,21,21,22,22,23,23,
	24,24,25,25,26,26,27,27,
	24,24,25,25,26,26,27,27,
	28,28,29,29,30,30,31,31,
	28,28,29,29,30,30,31,31
};

NNUE::NNUE() {
    m_isLoaded = false;
    m_filepath = "network-20220625.nnue";

    for(int i = 0; i < NNUE_SIZE; i++) {
        m_accumulator[0][i] = 0;
        m_accumulator[1][i] = 0;
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

    NetworkStorage* nnue_storage = new NetworkStorage;
    file.read((char*)nnue_storage, sizeof(NetworkStorage));

    for(size_t i = 0; i < (sizeof(nnue_storage->w1) / sizeof(nnue_storage->w1[0])); i++) {
        m_network.w1[i] = (float)nnue_storage->w1[i] / CONVERSION_FACTOR;
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

    delete nnue_storage;
    file.close();
}

#include "Utils.h"
Utils::Clock clock_eval;

int NNUE::Evaluate(int color) {
    //Layer 1
    float o1[ ARCH[L2][ROW] ];
    for(uint i = 0; i < NNUE_SIZE; i++) {
        o1[i            ] = Clamp(m_accumulator[color][i]);
    }
    for(uint i = 0; i < NNUE_SIZE; i++) {
        o1[i + NNUE_SIZE] = Clamp(m_accumulator[1-color][i]);
    }

    //Layers 2,3,4
    float o2[ ARCH[L3][ROW] ];
    float o3[ ARCH[L4][ROW] ];
    float o4[1];

    ComputeLayer(o1, o2, m_network.b2, m_network.w2, ARCH[L2][ROW], ARCH[L2][COL], true);
    ComputeLayer(o2, o3, m_network.b3, m_network.w3, ARCH[L3][ROW], ARCH[L3][COL], true);
    ComputeLayer(o3, o4, m_network.b4, m_network.w4, ARCH[L4][ROW], ARCH[L4][COL], false);

    return o4[0] * 100;
}

void NNUE::SavePosition(int ply) {
    std::memcpy(&m_backupAccumulator[ply], &m_accumulator, sizeof(m_accumulator));
}

void NNUE::RestorePosition(int ply) {
    std::memcpy(&m_accumulator, &m_backupAccumulator[ply], sizeof(m_backupAccumulator[ply]));
}

void NNUE::SetPieces(int color, uint64_t& pieces) {
    m_pieces[color] = &pieces;
}

void NNUE::Inputs_FullUpdate() {
    for(int i=0; i < NNUE_SIZE; i++) {
        m_accumulator[0][i] = m_network.b1[i];
        m_accumulator[1][i] = m_network.b1[i];
    }

    for(int color = WHITE; color <= BLACK; color++) {
        for(int pieceType = PAWN; pieceType <= QUEEN; pieceType++) {
            Bitboard bitboard = m_pieces[color][pieceType];
            while(bitboard) {
                int square = ResetLsb(bitboard);
                Inputs_AddPiece(color, pieceType-1, square);
            }
        }
    }
}

void NNUE::Inputs_AddPiece(int color, int pieceType, int square) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ 56;

    const int kingBucket_w = KING_BUCKETS[kingSquare_w];
    const int kingBucket_b = KING_BUCKETS[kingSquare_b];

    const int index_w = (pieceType * 2) + (color);
	const int index_b = (pieceType * 2) + (1 - color);

    const int square_w = square;
	const int square_b = square ^ 56;

    const int feature_w = (640 * kingBucket_w) + (64 * index_w) + (square_w);
	const int feature_b = (640 * kingBucket_b) + (64 * index_b) + (square_b);

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    for(int i = 0; i < NNUE_SIZE; i++) {
        m_accumulator[0][i] += m_network.w1[NNUE_SIZE * feature_w + i];
        m_accumulator[1][i] += m_network.w1[NNUE_SIZE * feature_b + i];
    }
}

void NNUE::Inputs_RemovePiece(int color, int pieceType, int square) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ 56;

    const int kingBucket_w = KING_BUCKETS[kingSquare_w];
    const int kingBucket_b = KING_BUCKETS[kingSquare_b];

    const int index_w = (pieceType * 2) + (color);
	const int index_b = (pieceType * 2) + (1 - color);

    const int square_w = square;
	const int square_b = square ^ 56;

    const int feature_w = (640 * kingBucket_w) + (64 * index_w) + (square_w);
	const int feature_b = (640 * kingBucket_b) + (64 * index_b) + (square_b);

    assert(feature_w <= NNUE_FEATURES);
    assert(feature_b <= NNUE_FEATURES);

    for(int i = 0; i < NNUE_SIZE; i++) {
        m_accumulator[0][i] -= m_network.w1[NNUE_SIZE * feature_w + i];
        m_accumulator[1][i] -= m_network.w1[NNUE_SIZE * feature_b + i];
    }
}

void NNUE::Inputs_MovePiece(int color, int pieceType, int fromSq, int toSq) {
    const int kingSquare_w = BitscanForward(m_pieces[WHITE][KING]);
    const int kingSquare_b = BitscanForward(m_pieces[BLACK][KING]) ^ 56;

    const int kingBucket_w = KING_BUCKETS[kingSquare_w];
    const int kingBucket_b = KING_BUCKETS[kingSquare_b];

    const int index_w = (pieceType * 2) + (color);
	const int index_b = (pieceType * 2) + (1 - color);

    const int fromSq_w = fromSq;
	const int fromSq_b = fromSq ^ 56;

    const int toSq_w = toSq;
	const int toSq_b = toSq ^ 56;

    const int feature_from_w = (640 * kingBucket_w) + (64 * index_w) + (fromSq_w);
	const int feature_from_b = (640 * kingBucket_b) + (64 * index_b) + (fromSq_b);

    const int feature_to_w = (640 * kingBucket_w) + (64 * index_w) + (toSq_w);
	const int feature_to_b = (640 * kingBucket_b) + (64 * index_b) + (toSq_b);

    assert(feature_from_w <= NNUE_FEATURES);
    assert(feature_from_b <= NNUE_FEATURES);

    assert(feature_to_w <= NNUE_FEATURES);
    assert(feature_to_b <= NNUE_FEATURES);

    for(int i = 0; i < NNUE_SIZE; i++) {
        m_accumulator[0][i] -= m_network.w1[NNUE_SIZE * feature_from_w + i];
        m_accumulator[1][i] -= m_network.w1[NNUE_SIZE * feature_from_b + i];

        m_accumulator[0][i] += m_network.w1[NNUE_SIZE * feature_to_w + i];
        m_accumulator[1][i] += m_network.w1[NNUE_SIZE * feature_to_b + i];
    }
}

float NNUE::Clamp(float n) {
    const float min = 0;
    const float max = 1;
    if(n < min) return min;
    if(n > max) return max;
    return n;
}

//Horizontal sum of 8 floats (256-bits)
inline float HorizontalSum256(__m256 v) {
    const __m128 r4 = _mm_add_ps( _mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1) );
    const __m128 r2 = _mm_add_ps( r4, _mm_movehl_ps( r4, r4 ) );
    const __m128 r1 = _mm_add_ss( r2, _mm_movehdup_ps( r2 ) );
    return _mm_cvtss_f32(r1);
}

void NNUE::ComputeLayer(float* inputLayer, float* outputLayer, float* biases, float* weights, int dimInput, int dimOutput, bool with_ReLU) {
    for(int o = 0; o < dimOutput; o++) {
        float sum = biases[o];

        const int offset = o * dimInput;

        __m256 dot0 = _mm256_setzero_ps();
        __m256 dot1 = _mm256_setzero_ps();
        __m256 dot2 = _mm256_setzero_ps();
        __m256 dot3 = _mm256_setzero_ps();

        //_mm512_setzero_ps

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

        if(with_ReLU) {
            outputLayer[o] = Clamp(sum);
        } else {
            outputLayer[o] = sum;
        }
    }
}
