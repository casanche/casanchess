#include "Heuristics.h"

#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "MoveScorer.h"
#include "Search.h"

#include <algorithm>

//Unnamed namespace (private functions)
namespace {

    void RateMoves(Board &board, MoveList &moves, TT& tt, const Heuristics &heuristics, int ply) {
        Move hashMove;
        TTEntry* ttEntry = tt.Probe(board.ZKey(), 0); //shallowest
        if(ttEntry)
            hashMove = ttEntry->bestMove;

        Move killer1 = heuristics.killer.Primary(ply);
        Move killer2 = heuristics.killer.Secondary(ply);
        Move killer3, killer4;
        if(ply >= 2) {
            killer3 = heuristics.killer.Primary(ply-2);
            killer4 = heuristics.killer.Secondary(ply-2);
        }

        for(auto& move : moves) {

            // Hash move: max score
            if(move == hashMove) {
                move.SetScore(Scorer::HASH);
                continue;
            }

            // Promotions
            if(move.IsPromotion()) {
                if(move.MoveType() == PROMOTION_CAPTURE && move.PromotionType() == PROMOTION_QUEEN) {
                    move.SetScore(Scorer::PROMOTION_CAPTURE);
                    continue;
                }
                else if(move.MoveType() == PROMOTION && move.PromotionType() == PROMOTION_QUEEN) {
                    move.SetScore(Scorer::PROMOTION);
                    continue;
                }
                else {
                    move.SetScore(Scorer::UNDERPROMOTION);
                    continue;
                }
            }

            // Captures
            if( move.CapturedType() ) {
                int see = board.SEE(move);
                if(see > 0)
                    move.SetScore( Scorer::ScoreFromSEE(see) );
                else if(see == 0) {
                    int captureHistoryScore = heuristics.captureHistory.Get(move, board.ActivePlayer());
                    int captureHistoryMax = heuristics.captureHistory.MaxValue();
                    u8 score = Scorer::ScoreFromHistory(Scorer::NEUTRALCAPTURE_MIN, Scorer::NEUTRALCAPTURE_MAX, captureHistoryScore, captureHistoryMax);
                    move.SetScore(score);
                } else {
                    u8 scoreSEE = Scorer::ScoreFromSEE(see);

                    int captureHistoryScore = heuristics.captureHistory.Get(move, board.ActivePlayer());
                    int captureHistoryMax = heuristics.captureHistory.MaxValue();
                    u8 scoreHist = Scorer::ScoreFromHistory_v2(Scorer::NEGATIVECAPTURE_MIN, Scorer::NEGATIVECAPTURE_MAX, captureHistoryScore, captureHistoryMax);
                    
                    move.SetScore( std::max(scoreSEE, scoreHist) );
                }
                continue;
            }

            //Killer heuristics
            if(move == killer1) {
                move.SetScore(Scorer::KILLER_1);
                continue;
            }
            if(move == killer2) {
                move.SetScore(Scorer::KILLER_2);
                continue;
            }
            if(move == killer3) {
                move.SetScore(Scorer::KILLER_3);
                continue;
            }
            if(move == killer4) {
                move.SetScore(Scorer::KILLER_4);
                continue;
            }

            // History heuristics
            int historyValue = heuristics.history.Get(move, board.ActivePlayer());
            int historyMax = heuristics.history.MaxValue();
            int score = Scorer::ScoreFromHistory(Scorer::HISTORY_MIN, Scorer::HISTORY_MAX, historyValue, historyMax);
            move.SetScore(score);
        }
    }

    void RateTactical(Board &board, MoveList &moves) {
        for(auto& move : moves) {
            if(move.MoveType() == PROMOTION_CAPTURE) {
                move.SetScore(Scorer::TACTICAL_PROMOTION_CAPTURE);
            } else if(move.MoveType() == PROMOTION) {
                move.SetScore(Scorer::TACTICAL_PROMOTION_NORMAL);
            } else if( move.CapturedType() ) {
                int see = board.SEE(move);
                move.SetScore( Scorer::TacticalScoreFromSEE(see) );
            }
        }
    }

    void RateEvasions([[maybe_unused]] Board &board, MoveList &moves) {
        // Captures > Other moves
        for(auto& move : moves) {
            if(move.CapturedType())
                move.SetScore(Scorer::EVASION_CAPTURE);
            else
                move.SetScore(Scorer::EVASION_NORMAL);
        }
    }

    bool ByScore(const Move &lmove, const Move &rmove) {
        const u8 lscore = lmove.Score();
        const u8 rscore = rmove.Score();

        // First compare by score (descending)
        if (lscore != rscore)
            return lscore > rscore;
        
        // For moves with the same score, use the move as tiebreaker
        return lmove.MoveAsNumber() > rmove.MoveAsNumber();
    }

} //unnamed namespace

void Sorting::SortMoves(Board &board, MoveList& moves, TT& tt, const Heuristics &heuristics, int ply) {
    RateMoves(board, moves, tt, heuristics, ply);
    std::ranges::sort(moves, ByScore);
}

void Sorting::SortEvasions(Board &board, MoveList& moves) {
    RateEvasions(board, moves);
    std::ranges::sort(moves, ByScore);
}

void Sorting::SortTactical(Board &board, MoveList& moves) {
    RateTactical(board, moves);
    std::ranges::sort(moves, ByScore);
}
