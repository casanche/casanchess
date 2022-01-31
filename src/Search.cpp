#include "Search.h"

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Uci.h"
using namespace Sorting;

#include <algorithm> //max()
#include <cmath> //INFINITY
#include <iomanip> //debug output

const bool TURNOFF_ASPIRATION_WINDOW = false;
const int ASPIRATION_WINDOW_DEPTH = 4;
const int ASPIRATION_WINDOW = 25;

const bool TURNOFF_NULLMOVE_PRUNING = false;
const int NULLMOVE_REDUCTION_FACTOR = 3;

const bool TURNOFF_LMR = false;
const bool TURNOFF_FUTILITY = false;

const int UCI_OUTPUT_CURRMOVE_MINTIME = 1000; //ms

#define DRAW_SCORE(ply) (ply & 1 ? 10 : -10)

Search::Search() {
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = 3500;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    m_bestScore = -INFINITE_SCORE;

    m_searchCount = 0;
    m_debugMode = false;

    ClearSearch();
    m_heuristics.history.Clear();

    Hash::tt.Clear();
    Hash::pawnHash.Clear();
}

void Search::ClearSearch() {
    m_elapsedTime = 0;
    m_nodes = 0;
    m_nps = 0;

    m_stop = false;
    m_nodesTimeCheck = 0;

    m_ply = 0;
    m_plyqs  = 0;
    m_selPly = 0;
    m_nullmoveAllowed = true;

    m_heuristics.killer.Clear();
}

void Search::IterativeDeepening(Board &board) {
    m_searchCount++;

    m_clock.Start();
    ClearSearch();
    m_heuristics.history.Age();

    for(m_depth = 1; m_depth <= m_maxDepth; m_depth++) {
        assert(m_ply == 0);
        assert(m_plyqs == 0);
        assert(m_nullmoveAllowed);

        //m_ply = 0;
        m_selPly = 0;
        int alpha, beta, score;

        //Aspiration window
        bool aspiration = !TURNOFF_ASPIRATION_WINDOW && m_depth >= ASPIRATION_WINDOW_DEPTH;
        if(aspiration) {
            alpha = m_bestScore - ASPIRATION_WINDOW;
            beta  = m_bestScore + ASPIRATION_WINDOW;
        } else {
            alpha = -INFINITE_SCORE;
            beta  =  INFINITE_SCORE;
        }

        score = RootMax(board, m_depth, alpha, beta);
        
        //Out of aspiration bounds. Repeat search with infinite limits
        while(score <= alpha || score >= beta) {
            alpha = -INFINITE_SCORE;
            beta  =  INFINITE_SCORE;
            score = RootMax(board, m_depth, alpha, beta);

            D( m_debug.Increment("IterativeDeepening Out of AspirationWindow") );
        }

        if(m_stop)
            break;
        m_elapsedTime = ElapsedTime();
        m_nps = static_cast<int>(1000 * m_nodes / (m_elapsedTime+1));

        //PV
        std::string PV;
        m_ponderMove = Move();
        Board newBoard = board;
        assert(newBoard == board);
        for(int depth = 1; depth <= m_depth; depth++) {
            TTEntry *ttEntry = Hash::tt.ProbeEntry(newBoard.ZKey(), 0);
            if(!ttEntry) continue;
            if(ttEntry->bestMove.MoveType() == NULLMOVE) break;

            Move bestMove = ttEntry->bestMove;
            PV += bestMove.Notation();
            PV += " ";

            if(depth == 2)
                m_ponderMove = bestMove;

            newBoard.MakeMove(bestMove);
        }

        if(UCI_OUTPUT)
            UciOutput(PV);

        if(m_elapsedTime > (m_allocatedTime / 2)) //check
             break;

        D( m_debug.Print() );
    }

    if(UCI_OUTPUT) {
        std::cout << "bestmove " << m_bestMove.Notation();
        if(UCI_PONDER)
            std::cout << " ponder " << m_ponderMove.Notation();
        std::cout << std::endl;
    }
}
void Search::UciOutput(std::string PV) {
    std::cout << "info depth " << m_depth;
    std::cout << " seldepth " << m_selPly;
    if(IsMateValue(m_bestScore)) {
        int mateScore = (m_bestScore > 0) ?  MATESCORE - m_bestScore + 1
                                          : -MATESCORE - m_bestScore - 1;
        std::cout << " score mate " << mateScore / 2; //return mate in moves, not in plies
    } else {
        std::cout << " score cp " << m_bestScore;
    }
    std::cout << " time " << m_elapsedTime;
    std::cout << " nodes " << m_nodes;
    std::cout << " nps " << m_nps;
    if(m_elapsedTime > 1000)
        std::cout << " hashfull " << Hash::tt.OccupancyPerMil();
    std::cout << " pv " << PV;
    std::cout << std::endl;
}

//Set limits
void Search::FixDepth(int depth) {
    m_allocatedTime = INFINITE;
    m_maxDepth = depth;
}
void Search::FixTime(int time) {
    m_allocatedTime = INFINITE;
    m_forcedTime = time;
}
void Search::FixNodes(int nodes) {
    m_allocatedTime = INFINITE;
    m_forcedNodes = nodes;
}
void Search::Infinite() {
    m_allocatedTime = INFINITE;
}

int Search::RootMax(Board &board, int depth, int alpha, int beta) {
    Move bestMove;
    int score;

    int alphaOriginal = alpha;

    D( m_debug.Increment("RootMax Hit") );

    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    D( if(depth == 1) P("Number of moves in root position: " << moves.size()) );

    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    int moveNumber = 0;
    for(auto move : moves) {
        moveNumber++;

        //Uci output
        if(m_elapsedTime > UCI_OUTPUT_CURRMOVE_MINTIME) {
            std::cout << "info currmovenumber " << moveNumber;
            std::cout << " currmove " << move.Notation();
            std::cout << std::endl;
        }

        board.MakeMove(move);
        m_ply++; m_nodes++;

        score = -NegaMax(board, depth-1, -beta, -alpha);

        board.TakeMove(move);
        m_ply--;

        //New bestMove found
        if(score > alpha) {
            alpha = score;
            bestMove = move;

            if(score >= beta)
                break;
        }

        //Check limits
        if(m_stop || TimeOver()) {
            m_stop = true;
            break;
        }
    }

    //Fill transposition tables
    bool outOfLimits = alpha <= alphaOriginal || alpha >= beta;
    if(!m_stop && !outOfLimits) {
        m_bestMove = bestMove;
        m_bestScore = alpha;

        Hash::tt.AddEntry(board.ZKey(), m_bestScore, TTENTRY_TYPE::EXACT, m_bestMove, depth, m_ply, m_searchCount);
    }

    return alpha;
}

int Search::NegaMax(Board &board, int depth, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLIES);

    bool isPV = (beta - alpha) != 1;

    D( m_debug.Increment("NegaMax Hits (at start)") );

    // --------- Should I stop? -----------
    m_nodesTimeCheck++;
    if( m_stop || NodeLimit() || TimeOver() ) {
        m_stop = true;
        return 0;
    }

    // --------- Repetition draws and 50-move rule -----------
    if(board.IsRepetitionDraw(m_ply) || board.FiftyRule() >= 100) {
        D( m_debug.Increment("NegaMax RepetitionDraw") );
        return DRAW_SCORE(m_ply);
    }

    //---------- Mate distance pruning ------------- //Different for PV nodes?
    alpha = std::max(alpha, -MATESCORE + m_ply);
    beta = std::min(beta, +MATESCORE - m_ply - 1);
    if(alpha >= beta) {
        D( m_debug.Increment("MateDistancePruning Hits") );
        return alpha;
    }

    // --------- Check extension -----------
    int extension = 0;
    bool inCheck = board.IsCheck();
    if(inCheck) {
        extension++;
        D( m_debug.Increment("Extension Check") );
    }

    // --------- Quiescence search -----------
    if(depth <= 0 && !inCheck) { //check !inCheck??
        return QuiescenceSearch(board, alpha, beta);
    }

    D( m_debug.Increment("NegaMax Hits") );

    // --------- Transposition table probe --------
    Move bestMove; //for later storage in the Transposition Table
    int bestScore = -INFINITE_SCORE;
    int alphaOriginal = alpha; //for later calculation of TTENTRY_TYPE
    
    TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), depth);
    if(ttEntry && !isPV) {
        D( m_debug.Increment("TT Hits (in NegaMax)") );
        int score = Hash::tt.ScoreFromHash(ttEntry->score, m_ply);
        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && score >= alpha && score <= beta) )
        {
            D( m_debug.Increment("TT Cut-Offs (in NegaMax)") );
            return score;
        }
    }

    //Calculate evaluation once at start, for pruning purposes
    int eval = 0;
    if(!inCheck) {
        D( m_debug.Increment("NegaMax Calls to Evaluation") );
        eval = Evaluation::Evaluate(board);
    }

    // --- Static null-move pruning (aka Reverse futility) ---
    const int staticMargin = 125;
    if(depth <= 4 && !isPV && !inCheck) {
        int staticEval = eval - depth * staticMargin;
        if(staticEval >= beta) {
            D( m_debug.Increment("Static Null-Move Pruning - Depth " + std::to_string(depth)) );
            return staticEval;
        }
    }

    // --------- Razoring -------------
    if(depth == 3 && eval + 1150 <= alpha && !isPV && !extension && Evaluation::AreHeavyPieces(board))
    {
        D( m_debug.Increment("Razoring - Depth " + std::to_string(depth)) );
        depth--;
    }

    // --------- Null-move pruning -----------
    if(!TURNOFF_NULLMOVE_PRUNING
        && !inCheck
        && m_nullmoveAllowed
        && eval >= beta  //very good score
        && depth > 1
        // && depth >= NULLMOVE_REDUCTION_FACTOR + (depth / 5)  //enough depth
        && Evaluation::AreHeavyPieces(board)  //active player has pieces on the board (to avoid zugzwang in K+P endgames)
    ) {
        D( m_debug.Increment("NullMove Hits - Depth " + std::to_string(depth)) );

        board.MakeNull();
        m_ply++;
        m_nullmoveAllowed = false;

        int R = NULLMOVE_REDUCTION_FACTOR + (depth / 4);
        int nullDepth = depth - R;
        int nullScore = -NegaMax(board, nullDepth, -beta, -beta + 1);

        board.TakeNull();
        m_ply--;
        m_nullmoveAllowed = true;

        if(nullScore >= beta) {
            D( m_debug.Increment("NullMove Cut-offs - Depth " + std::to_string(depth)) );
            if(IsMateValue(nullScore))
                nullScore = beta;  //to avoid false mates in zugzwang
            Hash::tt.AddEntry(board.ZKey(), nullScore, TTENTRY_TYPE::LOWER_BOUND, Move(), nullDepth, m_ply, m_searchCount);
            return nullScore;
        }
    }
    //Allow non-consecutive null-move pruning
    m_nullmoveAllowed = true;

    // --------- Generate the moves -----------
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    D( m_debug.Increment("NegaMax GenerateMoves Hits") );

    //----- One-reply extension -------
    if(moves.size() == 1) {
        D( m_debug.Increment("Extension One-reply") );
        extension++;
    }

    // --------- Check for checkmate and stalemate -----------
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("NegaMax Checkmate") );
            return -MATESCORE + m_ply; //checkmate
        }
        else {
            D( m_debug.Increment("NegaMax Stalemate") );
            return DRAW_SCORE(m_ply); //stalemate
        }
    }

    // --------- Sort moves -----------
    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    // ------- Futility pruning --------
    //Prune quiet moves in the loop?
    bool doFutility = false;
    int futilityMargin = 0;
    if (depth <= 4 && !isPV && !inCheck && !IsMateValue(alpha) && !IsMateValue(beta)) {
        futilityMargin = 150 + depth * 150;
        if(eval + futilityMargin < alpha) {
            D( m_debug.Increment("Futility - Depth " + std::to_string(depth)) );
            doFutility = true;
        }
    }

    int moveNumber = 0;
    for(auto move : moves) {
        assert(move.MoveType());

        moveNumber++;
        int score, reduction = 0;
        int localExtension = 0;

#ifdef DEBUG_NEGAMAX
        P("  NegamaxLoop, ply " << m_ply << " move: " << move.Notation());
#endif

        // ------- Futility pruning --------
        //Don't prune: hash move, promotions, SEE > 0 captures
        const int SEE_ZERO = 240;
        if(doFutility && move.Score() <= SEE_ZERO) {
            D( m_debug.Increment("Futility - FutileMove - " + std::to_string(depth)) );
            if(eval + futilityMargin > bestScore) {
                bestScore = eval + futilityMargin;
            }
            break;
        }

        // ---- Pawn to 7th/8th extension -----
        // if(move.PieceType() == PAWN
        //     && RelativeRank(board.ActivePlayer(), move.ToSq()) >= RANK7
        // )
        //     extension++;

        // ----- Recapture extension ------
        if(isPV && !extension && move.MoveType() == CAPTURE
            && move.ToSq() == board.LastMove().ToSq()
            && move.CapturedType() == board.LastMove().CapturedType()
        ) {
            D( m_debug.Increment("Extension Recapture") );
            localExtension++;
        }

        //-------- Late Move Reductions ----------
        if( !TURNOFF_LMR
              && moveNumber > 1     //never reduce the first move
              && depth >= 2         //avoid negative depths
              && !inCheck           //not in check
        ) {
            reduction = LateMoveReductions((int)move.Score(), depth, moveNumber, isPV);
        }

        board.MakeMove(move);
        m_ply++; m_nodes++;

        // -------- Principal Variation Search -----------
        int fullDepth = depth - 1 + extension + localExtension;
        int reducedDepth = fullDepth - reduction;

        //PV move: full window, full depth
        //Other moves: zero window, reduced depth
        if(isPV && moveNumber == 1)
            score = -NegaMax(board, fullDepth, -beta, -alpha);
        else {
            score = -NegaMax(board, reducedDepth, -alpha-1, -alpha);

            //Reduced search failed high: re-search with full depth
            if(reduction && score > alpha) {
                score = -NegaMax(board, fullDepth, -alpha-1, -alpha);
            }
            //Score within window: New PV!
            if(score > alpha && score < beta) { //'score < beta' is needed in fail-soft schemes
                score = -NegaMax(board, fullDepth, -beta, -alpha);
            }
        }

        board.TakeMove(move);
        m_ply--;

        if(score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        if(score > alpha) {

            if(score >= beta) {
                D( m_debug.Increment("NegaMax Cutoffs (score >= beta)") );

                Hash::tt.AddEntry(board.ZKey(), score, TTENTRY_TYPE::LOWER_BOUND, move, depth, m_ply, m_searchCount);

                //update heuristics
                if( move.IsQuiet() ) {
                    m_heuristics.killer.Update(move, m_ply);
                    m_heuristics.history.GoodHistory(move, board.ActivePlayer(), depth);
                }

                return score;
            }

            alpha = score;
        }
        
    } //move loop

    if(bestMove.MoveType() != 0) {
        TTENTRY_TYPE type = (alpha > alphaOriginal) ? TTENTRY_TYPE::EXACT : TTENTRY_TYPE::UPPER_BOUND;
        Hash::tt.AddEntry(board.ZKey(), bestScore, type, bestMove, depth, m_ply, m_searchCount);
    }

    return bestScore;
}

int Search::QuiescenceSearch(Board &board, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLIES);

    D( m_debug.Increment("Quiescence Hits") );

    bool isPV = (beta - alpha) != 1;
    int bestScore = -INFINITE_SCORE;
    bool inCheck = board.IsCheck();

    //--------- Standpat -----------
    int standPat = 0;
    if(!inCheck) {
        standPat = Evaluation::Evaluate(board);

        if(standPat > alpha) {
            if(standPat >= beta)
                return standPat;
            alpha = standPat;
        }
        bestScore =  standPat;
    }
    
    // --------- Transposition table lookup -----------
    TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), 0);
    if(ttEntry && !isPV) {
        int score = Hash::tt.ScoreFromHash(ttEntry->score, m_ply);
        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && score >= alpha && score <= beta) )
        {
            D( m_debug.Increment("TT Hits (in Quiescence)") );
            return score;
        }
    }

    //-------- Generate moves ----------
    MoveGenerator gen;
    MoveList moves = inCheck ? gen.GenerateMoves(board)
                             : gen.GenerateCaptures(board);

    D( m_debug.Increment("Quiescence GenerateMoves") );

    //--------- Check for checkmate/stalemate ---------
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("Quiescence Checkmate") );
            return -MATESCORE + m_ply; //checkmate
        }
        else if( gen.GenerateMoves(board).empty() ) {
            D( m_debug.Increment("Quiescence Stalemate") );
            return DRAW_SCORE(m_ply); //stalemate
        }
    }

    inCheck ? SortEvasions(board, moves)
            : SortQuiescence(board, moves);

    for(auto move : moves) {

        if(!inCheck) {
            //Futility pruning depending on SEE
            if(move.MoveType() == CAPTURE) {
                const int SEE_ZERO = 127;  //score equivalent to SEE = 0
                const int deltaMargin = 90;

                if(move.Score() < SEE_ZERO) continue;

                int evalMargin = move.Score() > SEE_ZERO ? standPat + deltaMargin + SEE_MATERIAL_VALUES[move.CapturedType()]
                                                         : standPat + deltaMargin;
                if(evalMargin < alpha) {
                    if(evalMargin > bestScore)
                        bestScore = evalMargin;
                    continue;
                }
            }
        }
        
        D( Board bef = board );

        board.MakeMove(move);
        m_ply++; m_plyqs++; m_nodes++; m_selPly = std::max(m_selPly, m_ply);
        D( m_debug.Increment("Quiescence Nodes") );

        int score = -QuiescenceSearch(board, -beta, -alpha);
        
        board.TakeMove(move);
        m_ply--; m_plyqs--;

        D( Board aft = board );
        D( assert(bef == aft) );

        if(score > bestScore)
            bestScore = score;

        if(score > alpha) {
            if(score >= beta)
                return score;
            alpha = score;
        }
    }

    return bestScore;
}

bool Search::TimeOver() {
    //Calculate time every N nodes
    if(m_nodesTimeCheck > 5000) {
        m_nodesTimeCheck = 0;
        m_elapsedTime = ElapsedTime();
        if(m_elapsedTime > m_allocatedTime || m_elapsedTime > m_forcedTime)
            return true;
    }
    return false;
}

void Search::AllocateLimits(Board &board, Limits limits) {
    m_limits = limits;
    m_nodes = 0;
    m_clock.Start();

    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = INFINITE;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    if(limits.infinite) { Infinite(); return; }
    if(limits.depth)    { FixDepth(limits.depth); return; }
    if(limits.moveTime) { FixTime(limits.moveTime); return; }
    if(limits.nodes)    { FixNodes(limits.nodes); return; }
    limits.movesToGo = 20 + 20 * limits.ponderhit; //estimation of the remaining moves

    COLOR color = board.ActivePlayer();

    int myTime   = (color == WHITE) ? limits.wtime : limits.btime;
    int yourTime = (color == WHITE) ? limits.btime : limits.wtime;

    int myInc    = (color == WHITE) ? limits.winc  : limits.binc;

    m_allocatedTime = myTime / limits.movesToGo + myInc;

    if(m_debugMode)
        std::cout << "info string TimeAllocation " <<  myTime << " " << yourTime << " " << m_allocatedTime << std::endl;
}

int Search::LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV) {
    int reduction = 0;

    //History moves
    if(moveScore < 180) {
        float logscore = logf(moveScore+1);
        reduction = (int)floorf(
            -0.5 -0.2*logscore
            - 2*(isPV)
            + (2.0 - 0.3*logscore) * logf(depth)
            + (0.3 + 0.15*logscore) * logf(moveNumber)
            );
    }
    //SEE << 0
    else if(moveScore >= 181 && moveScore <= 184) {
        reduction = (int)floorf(0.5 - 0.4*(isPV) + 1.35*logf(depth) + 0.4*logf(moveNumber));
    }
    //SEE < 0
    else if(moveScore >= 185 && moveScore <= 189) {
        reduction = (int)floorf(-0.85 + 1.35*logf(depth) + 0.4*logf(moveNumber));
    }
    //Killers 2,3,4
    else if(moveScore >= 191 && moveScore <= 193 && !isPV) {
        reduction = (int)floorf(-1.85 + 0.5*logf(depth) + 1.65*logf(moveNumber));
    }

    reduction = std::min(4, std::max(0, reduction));
    return reduction;
}

void SearchDebug::Transform() {
    std::string colorGreen = "\033[1;92m";
    std::string colorBlack = "\033[0m";
    debugVariables[colorGreen+"NegaMax Calls to Evaluation (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax Calls to Evaluation"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"NegaMax Cutoffs (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax Cutoffs (score >= beta)"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"NegaMax GenerateMoves (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax GenerateMoves Hits"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"TT Hits (in NegaMax) (permil)"+colorBlack] = 1000 * (double)debugVariables["TT Hits (in NegaMax)"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"TT Hits (in Quiescence) (permil)"+colorBlack] = 1000 * (double)debugVariables["TT Hits (in Quiescence)"] / debugVariables["Quiescence Hits"];
}

void SearchDebug::Print() {
    Transform();
    for(auto variable : debugVariables) {
        P("\t " << variable.first << " " << variable.second);
    }
}
