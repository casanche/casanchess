#include "Heuristics.h"

#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "Search.h"

#include <algorithm>

//Unnamed namespace (private functions)
namespace {

    void RateMoves(Board &board, MoveList &moves, TT& tt, const Heuristics &heuristics, int ply) {
        Move hashMove;
        TTEntry* ttEntry = tt.Probe(board.ZKey(), 0); //shallowest
        if(ttEntry) {
            hashMove = ttEntry->bestMove;
        }

        Move killer1 = heuristics.killer.Primary(ply);
        Move killer2 = heuristics.killer.Secondary(ply);
        Move killer3, killer4;
        if(ply >= 2) {
            killer3 = heuristics.killer.Primary(ply-2);
            killer4 = heuristics.killer.Secondary(ply-2);
        }

        for(auto& move : moves) {

            //Hash move: 255 (max)
            if(move == hashMove) {
                move.SetScore(255);
                continue;
            }

            //Queen promotion captures: 254
            //Queen promotions: 253
            //Underpromotions: 0
            if(move.IsPromotion()) {
                if(move.MoveType() == PROMOTION_CAPTURE && move.PromotionType() == PROMOTION_QUEEN) {
                    move.SetScore(254); continue;
                }
                else if(move.MoveType() == PROMOTION && move.PromotionType() == PROMOTION_QUEEN) {
                    move.SetScore(253); continue;
                }
                else { //underpromotion
                    move.SetScore(0); continue;
                }
            }

            //SEE positive: [241, 249]
            //SEE neutral: 240
            //SEE negative: [181-189]
            const bool useSEE = 1;
            if(useSEE) {
                PIECE_TYPE capturedType = move.CapturedType();
                if(capturedType) {
                    int see = board.SEE(move);

                    if(see > 0) {
                        int score = ((see - 1) * 8 / 1024) + 241;
                        score = std::min(score, 249);
                        assert(score >= 241 && score <= 249);
                        move.SetScore(SafeCastU8(score)); continue;
                    }
                    else if(see == 0) {
                        move.SetScore(240); continue;
                    }
                    else if(see < 0) {
                        int score = ((see + 1) * 8 / 1024) + 189;
                        score = std::max(score, 181);
                        assert(score >= 181 && score <= 189);
                        move.SetScore(SafeCastU8(score)); continue;
                    }
                    
                }
            }
            else {
                //Captured piece: 200-236
                PIECE_TYPE capturedType = move.CapturedType();
                PIECE_TYPE pieceType = move.PieceType();
                if(capturedType) {
                    int score = 200 + 6*capturedType - pieceType; //200 + capturedType
                    assert(score >= 200 && score <= 236);
                    move.SetScore(SafeCastU8(score));
                    continue;
                }
            }

            //Killer heuristics: 190-194
            if(move == killer1) {
                move.SetScore(194);
                continue;
            }
            if(move == killer2) {
                move.SetScore(193);
                continue;
            }
            if(move == killer3) {
                move.SetScore(192);
                continue;
            }
            if(move == killer4) {
                move.SetScore(191);
                continue;
            }

            //History heuristics (1-180)
            int historyScore = heuristics.history.Get(move, board.ActivePlayer());
            int maxValue = heuristics.history.MaxValue();
            const int maxScore = 180;
            const int minScore = 1;
            historyScore = minScore + historyScore * (maxScore-minScore) / (maxValue+1);
            assert(historyScore >= minScore && historyScore <= maxScore);
            move.SetScore(SafeCastU8(historyScore));

        }
    }

    void RateTactical(Board &board, MoveList &moves) {
        const int TACTICAL_PROMOTION_CAPTURE_SCORE = 255;
        const int TACTICAL_PROMOTION_NORMAL_SCORE = 254;

        for(auto& move : moves) {
            if(move.MoveType() == PROMOTION_CAPTURE) {
                move.SetScore(TACTICAL_PROMOTION_CAPTURE_SCORE);
            } else if(move.MoveType() == PROMOTION) {
                move.SetScore(TACTICAL_PROMOTION_NORMAL_SCORE);
            } else if(move.MoveType() == CAPTURE || move.MoveType() == ENPASSANT) { // includes enpassant
                int see = board.SEE(move);
                move.SetScore( SEE::ToScore(see) );
            } else if(move.MoveType() == NORMAL) { //Checks
                move.SetScore( SEE::ToScore(0) );
            }
        }
    }

    void RateEvasions([[maybe_unused]] Board &board, MoveList &moves) {
        // Captures > Other moves
        for(auto& move : moves) {
            if(move.CapturedType())
                move.SetScore(1);
            else
                move.SetScore(0);
        }
    }

    [[maybe_unused]] bool MVV(const Move &lmove, const Move &rmove) {
        return lmove.CapturedType() > rmove.CapturedType();
    }
    [[maybe_unused]] bool MVVtoLVA(const Move &lmove, const Move &rmove) {
        if(lmove.CapturedType() > rmove.CapturedType()) //MVV
            return true;

        else if(lmove.CapturedType() == rmove.CapturedType()) {
            return lmove.PieceType() < rmove.PieceType(); //LVA
        }

        return false;
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

// Converts a 'tactical move' score to a 'see' value
int SEE::FromScore(u8 score) {
    assert(score >= MIN_SCORE && score <= MAX_SCORE);

    int score_offset = score - MIN_SCORE;
    int normalized_see = score_offset * SEE_RANGE / SCORE_RANGE;

    int see = normalized_see - SEE_MAX;

    assert(see >= -SEE_MAX && see <= SEE_MAX);
    return see;
}

// Converts a 'see' value from board.SEE() to a 'tactical move' score
u8 SEE::ToScore(int see) {
    int normalized_see = std::clamp(see, -SEE_MAX, SEE_MAX);
    normalized_see += SEE_MAX; // Range: (0, SEE_RANGE)

    int score = MIN_SCORE + normalized_see * SCORE_RANGE / SEE_RANGE;

    assert(score >= MIN_SCORE && score <= MAX_SCORE);
    return SafeCastU8(score);
}

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
