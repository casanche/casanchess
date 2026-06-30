#pragma once

#include "Board.h"
#include "Utils.h"

#include <string>

/// Configuration for random position generation.
/// Bias values: 0.0 = never allow, 1.0 = always allow when possible.
/// Default values produce positions similar to real games.
struct RandomPositionConfig {
    // Pawn structure biases
    float doubledPawnBias = 0.15f;     // Allow doubled pawns (same file)
    float tripledPawnBias = 0.05f;     // Allow tripled pawns
    float advancedPawnBias = 0.15f;    // Allow pawns on 7th rank
    
    // Piece biases
    float sameBishopsBias = 0.01f;     // Allow both bishops on same color squares
};

/// Generates random chess positions for NNUE training data.
/// Produces positions that complement real game data by including
/// rare structures (doubled pawns, etc.) with controlled frequency.
class RandomPositionGenerator {
public:
    explicit RandomPositionGenerator(uint64_t seed = 0);
    
    /// Generate a random position and set it on the board.
    /// Returns the simplified FEN string (pieces only, no castling/ep).
    std::string Generate(Board& board, const RandomPositionConfig& config = {});
    
private:
    Utils::PRNG m_rng;
    
    /// Check if a square is valid for placing a pawn.
    bool IsValidPawnSquare(const Board& board, COLOR color, int square, 
                           const RandomPositionConfig& config);
    
    /// Check if a square is valid for placing a piece (non-pawn).
    bool IsValidPieceSquare(const Board& board, COLOR color, int piece, int square,
                            const RandomPositionConfig& config);
    
    /// Get a random square biased towards the player's side of the board.
    int RandomSquareForColor(COLOR color);
};
