#include "MoveScorer.h"

#include <algorithm>

namespace Scorer {
    constexpr int SEE_MAX = SEE::MATERIAL_VALUES[QUEEN];
    constexpr int SEE_RANGE = SEE_MAX * 2;

    constexpr int TACTICAL_RANGE = TACTICAL_MAX - TACTICAL_MIN;
    constexpr int TACTICAL_NEUTRALCAPTURE = (TACTICAL_MIN + TACTICAL_MAX) / 2; // Equivalent to SEE = 0 for tactical scores
}

u8 Scorer::ScoreFromHistory(int historyValue, int historyMax) {   
    int historyScore = HISTORY_MIN + historyValue * (HISTORY_MAX-HISTORY_MIN) / (historyMax+1);
    
    assert(historyScore >= HISTORY_MIN && historyScore <= HISTORY_MAX);
    return SafeCastU8(historyScore);
}

u8 Scorer::ScoreFromSEE(int see) {
    if(see > 0) {
        constexpr int SCORE_RANGE = POSITIVECAPTURE_MAX - POSITIVECAPTURE_MIN;

        int normalized_see = std::clamp(see, 0, SEE_MAX);
        int score = POSITIVECAPTURE_MIN + normalized_see * SCORE_RANGE / SEE_MAX;
        
        assert(score >= POSITIVECAPTURE_MIN && score <= POSITIVECAPTURE_MAX);
        return SafeCastU8(score);
    }
    else if(see == 0) {
        return NEUTRALCAPTURE;
    }
    else { // see < 0
        constexpr int SCORE_RANGE = NEGATIVECAPTURE_MAX - NEGATIVECAPTURE_MIN;

        int normalized_see = std::clamp(see, -SEE_MAX, 0);
        int score = NEGATIVECAPTURE_MAX + normalized_see * SCORE_RANGE / SEE_MAX;
        
        assert(score >= NEGATIVECAPTURE_MIN && score <= NEGATIVECAPTURE_MAX);
        return SafeCastU8(score);
    }
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
