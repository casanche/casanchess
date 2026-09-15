// Ambition.cpp
//
// Favors positions with a lower estimated draw probability.

#include "Ambition.h"

#include "Constants.h"
#include "NNUE.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
    constexpr int LOGIT_SCALE = 100;
    constexpr int RESIDUAL_LIMIT = 2 * LOGIT_SCALE;

    // Draw baseline for network-20260910.nnue
    // logit(P(draw)) = 1.078608 - 1.294259 * abs(eval / 100)
    constexpr int BASELINE_INTERCEPT = 108;
    constexpr int BASELINE_SLOPE = 1294;
    constexpr int BASELINE_SLOPE_SCALE = 1000;

    constexpr int PROBABILITY_SCALE = 32768;
    // Probabilities beyond +/-12 round to zero or one at this scale
    constexpr int SIGMOID_LIMIT = 12 * LOGIT_SCALE;
    constexpr int SIGMOID_SIZE = 2 * SIGMOID_LIMIT + 1;
    constexpr int MAX_DRAW_PROBABILITY_DELTA = 15143; // round(32768 * tanh(0.5))

    constexpr int DivideRounded(int value, int divisor) {
        return (value + divisor / 2) / divisor;
    }

    std::array<u16, SIGMOID_SIZE> BuildDrawProbabilityTable() {
        std::array<u16, SIGMOID_SIZE> table{};
        for(int scaledLogit = -SIGMOID_LIMIT; scaledLogit <= SIGMOID_LIMIT; scaledLogit++) {
            const double probability = 1.0 / (
                1.0 + std::exp(-scaledLogit / static_cast<double>(LOGIT_SCALE))
            );
            table[scaledLogit + SIGMOID_LIMIT] = static_cast<u16>(
                std::round(probability * PROBABILITY_SCALE)
            );
        }
        return table;
    }

    // Lookup-table for: 1 / (1 + exp(-logit))
    const std::array<u16, SIGMOID_SIZE> DRAW_PROBABILITY_TABLE = BuildDrawProbabilityTable();

    int DrawProbability(int scaledLogit) {
        scaledLogit = std::clamp(scaledLogit, -SIGMOID_LIMIT, SIGMOID_LIMIT);
        return DRAW_PROBABILITY_TABLE[scaledLogit + SIGMOID_LIMIT];
    }

    int DrawAdjustment(const EvaluationOutput& output, int maximumAdjustment) {
        const int residual = std::clamp(output.drawishness, -RESIDUAL_LIMIT, RESIDUAL_LIMIT);
        const int baseline = BASELINE_INTERCEPT
            - DivideRounded(BASELINE_SLOPE * std::abs(output.eval), BASELINE_SLOPE_SCALE);
        const int probabilityDelta = DrawProbability(baseline + residual) - DrawProbability(baseline);

        // Scale the probability change to an adjustment between -Ambition and +Ambition
        return maximumAdjustment * probabilityDelta / MAX_DRAW_PROBABILITY_DELTA;
    }
}

int Ambition::Apply(const EvaluationOutput& output, int maximumAdjustment, bool rootPlayerToMove) {
    const int adjustment = DrawAdjustment(output, maximumAdjustment);
    const int rootSign = rootPlayerToMove ? 1 : -1;

    const int adjustedEval = output.eval - rootSign * adjustment;
    return std::clamp(adjustedEval, -WINSCORE + 1, WINSCORE - 1);
}
