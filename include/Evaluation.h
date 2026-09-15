#pragma once

#include "Constants.h"
#include "NNUE.h"

class Board;

namespace Evaluation {

    int Evaluate(const Board& board, COLOR rootPlayer, int ambition);
    EvaluationOutput EvaluateOutputs(const Board& board);

} // namespace Evaluation
