#pragma once

struct EvaluationOutput;

namespace Ambition {
    // Adjust evaluation from the root player's perspective
    int Apply(const EvaluationOutput& output, int maximumAdjustment, bool rootPlayerToMove);
}
