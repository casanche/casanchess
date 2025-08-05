#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Constants.h"
#include "Move.h"

#include <array>
#include <span>

class Board;
struct MoveGenContext;

const int MAX_MOVES_RESERVE = 256;
using MoveList = std::array<Move, MAX_MOVES_RESERVE>;
using MoveBuffer = std::span<Move>;

namespace MoveGenerator {
    // Generate all legal moves
    MoveBuffer GenerateMoves(Board &board, MoveBuffer moveBuffer);
    
    // Generate check evasions when in check
    MoveBuffer GenerateEvasionMoves(Board &board, MoveBuffer moveBuffer);
    // Generate tactical moves (captures and promotions)
    MoveBuffer GenerateTacticalMoves(Board &board, MoveBuffer moveBuffer);
    
    // Pick a random move from the move list
    Move RandomMove(MoveBuffer moveBuffer);
}

#endif //MOVEGENERATOR_H
