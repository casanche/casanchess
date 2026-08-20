#pragma once

#include "Board.h"
#include "Utils.h"

// Generation of random positions for NNUE training data
// These positions complement real games by including unbalanced structures

// Bias values (percent chance):
//      0   = never allow
//      100 = always allow when possible
// Default values produce positions similar to real games.
struct RandomPositionConfig {
    // Pawn structure biases
    uint doubledPawnBias = 12;          // Allow doubled pawns
    uint tripledPawnBias = 3;           // Allow tripled pawns
    uint advancedPawnBias = 20;         // Limit pawns on 7th

    // Piece biases
    uint sameBishopsBias = 1;           // Allow both bishops on same color squares
};

class RandomPositionGenerator {
public:
    explicit RandomPositionGenerator(uint64_t seed = 0);

    // Generate a random position and set it on the board.
    // Returns the complete FEN string.
    std::string GenerateV2(Board& board, const RandomPositionConfig& config = {});

private:
    int PopBiasedSquare(Bitboard& globalMask, Bitboard preferredMask, uint biasPercent);
    int PopRandomSquare(Bitboard& mask);

    Utils::PRNG m_rng;
};
