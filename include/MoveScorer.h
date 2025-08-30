#pragma once

#include "Constants.h"

namespace Scorer {

    // Normal positions
    constexpr int HASH = 255;
    constexpr int PROMOTION_CAPTURE = 254;
    constexpr int PROMOTION = 253;
    constexpr int POSITIVECAPTURE_MAX = 249;
    constexpr int POSITIVECAPTURE_MIN = 241;
    constexpr int NEUTRALCAPTURE = 240;
    constexpr int NEUTRALCAPTURE_MAX = 240;
    constexpr int NEUTRALCAPTURE_MIN = 240;
    constexpr int KILLER_1 = 194;
    constexpr int KILLER_2 = 193;
    constexpr int KILLER_3 = 192;
    constexpr int KILLER_4 = 191;
    constexpr int NEGATIVECAPTURE_MAX = 189;
    constexpr int NEGATIVECAPTURE_MIN = 181;
    constexpr int HISTORY_MAX = 180;
    constexpr int HISTORY_MIN = 1;
    constexpr int UNDERPROMOTION = 0;
    u8 ScoreFromHistory(int minScore, int maxScore, int historyValue, int historyMax);
    u8 ScoreFromSEE(int see);

    // Tactical positions
    constexpr int TACTICAL_PROMOTION_CAPTURE = 255;
    constexpr int TACTICAL_PROMOTION_NORMAL = 254;
    constexpr int TACTICAL_MAX = 253;
    constexpr int TACTICAL_MIN = 1;
    u8 TacticalScoreFromSEE(int see);
    int SEEFromTacticalScore(u8 score);

    // Check evasion positions
    constexpr int EVASION_CAPTURE = 1;
    constexpr int EVASION_NORMAL = 0;

    // Single methods

    inline bool IsNeutralCapture(int score) {
        return score == NEUTRALCAPTURE;
    }

    inline bool IsNegativeCapture(int score) {
        return score >= NEGATIVECAPTURE_MIN
            && score <= NEGATIVECAPTURE_MAX;
    }

    inline bool IsHistoryMove(int score) {
        return score >= HISTORY_MIN
            && score <= HISTORY_MAX;
    }

    // Composite methods
    
    inline bool IsNegativeOrNeutral(int score) {
        return IsNeutralCapture(score) || IsNegativeCapture(score);
    }
}
