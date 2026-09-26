#include "MoveScorer.h"

#include <algorithm>
#include <cmath>

namespace Scorer {
    constexpr int SEE_MAX = SEE::MATERIAL_VALUES[QUEEN];
    constexpr int SEE_RANGE = SEE_MAX * 2;

    constexpr int TACTICAL_RANGE = TACTICAL_MAX - TACTICAL_MIN;
}

u8 Scorer::ScoreFromHistory(int historyValue, int historyLimit) {
    assert(std::abs(historyValue) <= historyLimit);

    constexpr int SCORE_RANGE = HISTORY_MAX - HISTORY_MIN;
    int normalizedHistory = historyValue + historyLimit; // Shift to non-negative range
    int score = HISTORY_MIN + normalizedHistory * SCORE_RANGE / (2 * historyLimit);

    assert(score >= HISTORY_MIN && score <= HISTORY_MAX);
    return SafeCastU8(score);
}

u8 Scorer::ScoreFromSEE(int see) {
    constexpr int SCORE_RANGE = POSITIVECAPTURE_MAX - POSITIVECAPTURE_MIN;
    static_assert(SCORE_RANGE == (NEGATIVECAPTURE_MAX - NEGATIVECAPTURE_MIN), "Same range for positive and negative captures.");

    if(see > 0) {
        int normalized_see = std::clamp(see, 0, SEE_MAX);
        int score = POSITIVECAPTURE_MIN + normalized_see * SCORE_RANGE / SEE_MAX;
        
        assert(score >= POSITIVECAPTURE_MIN && score <= POSITIVECAPTURE_MAX);
        return SafeCastU8(score);
    }
    else if(see == 0) {
        return NEUTRALCAPTURE;
    }
    else { // see < 0
        int normalized_see = std::clamp(see, -SEE_MAX, 0);
        int score = NEGATIVECAPTURE_MAX + normalized_see * SCORE_RANGE / SEE_MAX;
        
        assert(score >= NEGATIVECAPTURE_MIN && score <= NEGATIVECAPTURE_MAX);
        return SafeCastU8(score);
    }
}

// Converts a capture score back to its 'see' value (middle of the bin)
int Scorer::SEEFromScore(u8 score) {
    constexpr int SCORE_RANGE = POSITIVECAPTURE_MAX - POSITIVECAPTURE_MIN;
    static_assert(SCORE_RANGE == (NEGATIVECAPTURE_MAX - NEGATIVECAPTURE_MIN), "Same range for positive and negative captures.");
    constexpr int BUCKET = SEE_MAX / SCORE_RANGE;

    if(IsNeutralCapture(score))
        return 0;

    if(IsNegativeCapture(score)) {
        return (score - NEGATIVECAPTURE_MAX) * BUCKET - BUCKET / 2;
    }

    // Positive captures
    assert(score >= POSITIVECAPTURE_MIN && score <= POSITIVECAPTURE_MAX);
    return (score - POSITIVECAPTURE_MIN) * BUCKET + BUCKET / 2;
}

// Converts a 'see' value to a 'tactical move' score
u8 Scorer::TacticalScoreFromSEE(int see) {
    int normalized_see = std::clamp(see, -SEE_MAX, SEE_MAX) + SEE_MAX;
    int score = TACTICAL_MIN + normalized_see * TACTICAL_RANGE / SEE_RANGE;

    assert(score >= TACTICAL_MIN && score <= TACTICAL_MAX);
    return SafeCastU8(score);
}

// Converts a 'tactical move' score to a 'see' value
int Scorer::SEEFromTacticalScore(u8 score) {
    assert(score >= TACTICAL_MIN && score <= TACTICAL_MAX);

    int score_offset = score - TACTICAL_MIN;
    int normalized_see = score_offset * SEE_RANGE / TACTICAL_RANGE;

    int see = normalized_see - SEE_MAX;

    assert(see >= -SEE_MAX && see <= SEE_MAX);
    return see;
}
