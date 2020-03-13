#include "Sorting.h"
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
            if(move.IsPromotion() && move.PromotionType() == PROMOTION_QUEEN) {
                if(move.MoveType() == PROMOTION_CAPTURE) {
                    move.SetScore(254); continue;
                } else {
                    move.SetScore(253); continue;
                }
            }

            //SEE positive: 251
            //SEE neutral: 250
            //SEE negative: 181
            const bool useSEE = 1;
            if(useSEE) {
                PIECE_TYPE capturedType = move.CapturedType();
                if(capturedType) {
                    int see = board.SEE(move);
                    if(see > 0) {
                        move.SetScore(251); continue;
                    } else if(see == 0) {
                        move.SetScore(250); continue;
                    } else if(see < 0) {
                        move.SetScore(181); continue;
                    }
                }
            }
            else {
                //Captured piece: 200-236
                PIECE_TYPE capturedType = move.CapturedType();
                PIECE_TYPE pieceType = move.PieceType();
                if(capturedType) {
                    move.SetScore(200 + 6*capturedType - pieceType); //200 + capturedType
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

            //History heuristics (0-180)
            int heurScore = heuristics.history.Get(move, board.ActivePlayer());
            int maxValue = heuristics.history.MaxValue();
            if(maxValue > 180) {
                heurScore *= 180 / (maxValue+1);
            }
            move.SetScore(heurScore);
            continue;

            // Quiet pieces 150-200 from Psqt
            // int psqtScore = Evaluation::PIECE_SQUARE_TABLE[move.PieceType()][move.ToSq()];
            // U8 score = 175 + (psqtScore) / 2;
            // move.SetScore(score);
            // continue;

        }
    }

    void RateQuiescence(Board &board, MoveList& moveList) {
        for(auto &move : moveList) {
            if(move.CapturedType()) {
                int see = board.SEE(move);

                if     (see > 1000) see = 1000;
                else if(see < -1000) see = -1000;
                see += 1000; //0-2000
                const float factor = 255.f / 2000;
                float score = see * factor;

                assert(score >= 0 && score <= 255);
                move.SetScore( (U8)score );
            }
        }
    }

}

//Unnamed namespace (private functions)
namespace {
    bool MVV(const Move &lmove, const Move &rmove) {
        return lmove.CapturedType() > rmove.CapturedType();
    }
    // bool MVVtoLVA(const Move &lmove, const Move &rmove) {
    //     if(lmove.CapturedType() > rmove.CapturedType()) //MVV
    //         return true;

    //     else if(lmove.CapturedType() == rmove.CapturedType()) {
    //         return lmove.PieceType() < rmove.PieceType(); //LVA
    //     }

    //     return false;
    // }
    bool ByScore(const Move &lmove, const Move &rmove) {
        return lmove.Score() > rmove.Score();
    }
    bool ByNodes(const RootMove &lhs, const RootMove &rhs) {
        return lhs.nodeCount > rhs.nodeCount;
    }
}

//Sorting namespace (public interface)
void Sorting::SortCaptureMoves(Board &board, MoveList &moveList) {
    const bool useSEE = 1;
    if(useSEE) {
        RateQuiescence(board, moveList);
        std::sort(moveList.begin(), moveList.end(), ByScore);
    }
    else {
        std::sort(moveList.begin(), moveList.end(), MVV);
    }
}

void Sorting::SortMoves(Board &board, MoveList& moveList, TT& tt, const Heuristics &heuristics, int ply) {
    RateMoves(board, moveList, tt, heuristics, ply);

    std::sort(moveList.begin(), moveList.end(), ByScore);
}

void Sorting::SortRootMoves(std::vector<RootMove>& rootMoves) {
    std::sort(rootMoves.begin(), rootMoves.end(), ByNodes);
}
