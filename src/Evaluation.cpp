// Evaluation.cpp
//
// Position evaluation. Returns a score in centipawns from the side to move's perspective.
// NNUE provides the objective evaluation and drawishness output; Ambition optionally
// adjusts the score from the root player's perspective.

#include "Evaluation.h"

#include "Ambition.h"
#include "Board.h"
#include "BitboardUtils.h"

namespace Evaluation {

    namespace {

        // K vs K, K+N/B vs K
        bool InsufficientMaterial(const Board& board) {
            const int pieces = PopCount(board.AllPieces());
            return pieces == 2 || (
                pieces == 3
                && ( board.Piece(WHITE, KNIGHT) | board.Piece(WHITE, BISHOP)
                   | board.Piece(BLACK, KNIGHT) | board.Piece(BLACK, BISHOP) ) != 0
            );
        }

        int EvaluateObjective(const Board& board) {
            if(InsufficientMaterial(board))
                return 0;

            return board.NNUE_Evaluate();
        }

    } // namespace

    int Evaluate(const Board& board, COLOR rootPlayer, int ambition) {
        if(ambition == 0)
            return EvaluateObjective(board);

        return Ambition::Apply(
            EvaluateOutputs(board),
            ambition,
            board.ActivePlayer() == rootPlayer
        );
    }

    EvaluationOutput EvaluateOutputs(const Board& board) {
        if(InsufficientMaterial(board))
            return {0, 0};

        return board.NNUE_EvaluateOutputs();
    }

} // namespace Evaluation
