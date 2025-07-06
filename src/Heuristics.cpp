#include "Heuristics.h"
#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "Search.h"
#include <algorithm>

//Unnamed namespace (private functions)
namespace {

    void RateMoves(Board &board, MoveList& moveList, TT& tt, const Heuristics &heuristics, int ply) {
        Move hashMove;
        TTEntry* ttEntry = tt.ProbeEntry(board.ZKey(), 0); //shallowest
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

        for(auto &move : moveList) {

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

    void RateQuiescence(Board &board, MoveList& moveList) {
        for(auto &move : moveList) {
            if(move.IsPromotion()) {
                move.SetScore(255);
                continue;
            }
            if(move.IsCapture()) {
                int see = board.SEE(move);

                if     (see > 1000) see = 1000;
                else if(see < -1000) see = -1000;
                see += 1000; //0-2000

                const int maxValue = 2000;
                const int maxScore = 254;
                const int minScore = 1;
                int score = minScore + see * (maxScore-minScore) / (maxValue+1);

                assert(score >= 1 && score <= 254);
                move.SetScore((u8)score);
                continue;
            }
        }
    }

    void RateEvasions([[maybe_unused]] Board &board, MoveList& moveList) {
        // Captures > Other moves
        for(auto &move : moveList) {
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
        return lmove.Score() > rmove.Score();
    }

} //unnamed namespace

//Public Interface
void Sorting::SortQuiescence(Board &board, MoveList &moveList) {
    RateQuiescence(board, moveList);
    std::sort(moveList.begin(), moveList.end(), ByScore);
}

void Sorting::SortEvasions(Board &board, MoveList &moveList) {
    RateEvasions(board, moveList);
    std::sort(moveList.begin(), moveList.end(), ByScore);
}

void Sorting::SortMoves(Board &board, MoveList& moveList, TT& tt, const Heuristics &heuristics, int ply) {
    RateMoves(board, moveList, tt, heuristics, ply);

    std::sort(moveList.begin(), moveList.end(), ByScore);
}
