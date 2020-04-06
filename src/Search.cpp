#include "Search.h"

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Sorting.h"
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

const int STATIC_MARGIN[5] = {0, 150, 300, 500, 700};

Search::Search() {
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = 3500;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    m_bestScore = -INFINITE_SCORE;

    m_counter = 0;
    m_debugMode = false;

    ClearSearch();
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
    m_selPly = 0;
    m_nullmoveAllowed = true;

    m_heuristics.killer.Clear();
    m_heuristics.history.Clear(); //or Age()
}

void Search::IterativeDeepening(Board &board) {
    m_counter++;
    m_startTime = Clock::now();
    ClearSearch();

    for(m_depth = 1; m_depth <= m_maxDepth; m_depth++) {
        m_ply = 0;
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

        //Filling variables
        m_elapsedTime = ElapsedTime();
        m_nps = static_cast<int>(1000 * m_nodes / (m_elapsedTime+1));

        //PV
        std::string PV;
        Board newBoard = board;
        assert(newBoard == board);
        for(int depth = 1; depth <= m_depth; depth++) {
            TTEntry *ttEntry = Hash::tt.ProbeEntry(newBoard.ZKey(), 0);
            if(!ttEntry) continue;
            if(ttEntry->bestMove.MoveType() == NULLMOVE) break;

            Move bestMove = ttEntry->bestMove;
            PV += bestMove.Notation();
            PV += " ";

            newBoard.MakeMove(bestMove);
        }

        //Uci output
        UciOutput(PV);

        if(m_elapsedTime > (m_allocatedTime / 2))
             break;

        D( m_debug.Print() );
    }

    std::cout << "bestmove " << m_bestMove.Notation() << std::endl;
}
void Search::UciOutput(std::string PV) {
    std::cout << "info depth " << m_depth;
    std::cout << " seldepth " << m_selPly;
    if(IsMateValue(m_bestScore)) {
        int mateScore;
        if     (m_bestScore > 0) mateScore =  MATESCORE - m_bestScore + 1; //ply+1
        else if(m_bestScore < 0) mateScore = -MATESCORE - m_bestScore - 1; //-ply-1
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

int Search::RootMax(Board &board, int depth, int alpha, int beta) {
    Move bestMove;
    int score;

    int alphaOrig = alpha;

    D( m_debug.Increment("RootMax Hit") );

    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    D( if(depth == 1) P("Number of moves in root position: " << moves.size()) );

    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    int count = 0;
    for(size_t i = 0; i < moves.size(); i++) {
        count++;

        Move move = moves[i];

        //Uci output
        if(m_elapsedTime > 1000) {
            std::cout << "info currmovenumber " << count;
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
    bool outOfLimits = alpha <= alphaOrig || alpha >= beta;
    if(!m_stop && !outOfLimits) {
        m_bestMove = bestMove;
        m_bestScore = alpha;

        Hash::tt.AddEntry(board.ZKey(), m_bestScore, TTENTRY_TYPE::EXACT, m_bestMove, depth, m_ply, m_counter);
    }

    return alpha;
}

int Search::NegaMax(Board &board, int depth, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);
    assert(depth >= 0);

    bool isPV = (beta - alpha) != 1;

    D( m_debug.Increment("NegaMax Hits (at start)") );

    // --------- Should I stop? -----------
    m_nodesTimeCheck++;
    if( m_stop || TimeOver() ) {
        m_stop = true;
        return 0;
    }

    // --------- Repetition draws and 50-move rule -----------
    if(board.IsRepetitionDraw(m_ply) || board.FiftyRule() >= 100) {
        D( m_debug.Increment("NegaMax RepetitionDraw") );
        return 0;
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
        D( m_debug.Increment("NegaMax Extension Check") );
    }

    // --------- Quiescence search -----------
    if(depth == 0 && !inCheck) {
        return QuiescenceSearch(board, alpha, beta);
    }

    D( m_debug.Increment("NegaMax Hits") );

    // --------- Transposition table probe --------
    Move bestMove; //for later storage in the Transposition Table
    int bestScore = -INFINITE_SCORE;
    int alphaOriginal = alpha; //for later calculation of TTENTRY_TYPE
    
    TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), depth);
    if(ttEntry && !isPV) {
        int score = Hash::tt.ScoreFromHash(ttEntry->score, m_ply);
        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && score >= alpha && score <= beta) )
        {
            D( m_debug.Increment("TT Hits (in NegaMax)") );
            return score;
        }
    }

    //Calculate evaluation once at start, for pruning purposes
    int eval;
    if(!inCheck)
        eval = Evaluation::Evaluate(board);

    D( m_debug.Increment("NegaMax Calls to Evaluation") );

    // --- Static null-move pruning (aka Reverse futility) ---
    const int staticMargin = 125;
    if(depth <= 4 && !isPV && !inCheck && Evaluation::AreHeavyPieces(board)) {
        int staticEval = eval - depth * staticMargin;
        if(staticEval >= beta) {
            D( m_debug.Increment("Static Null-Move Pruning (depth " + std::to_string(depth) + ")") );
            return staticEval;
        }
    }

    // --------- Razoring -------------
    if(depth == 3 && eval + 1150 <= alpha && !isPV && !extension && Evaluation::AreHeavyPieces(board))
    {
        depth--;
    }

    // --------- Null-move pruning -----------
    if(!TURNOFF_NULLMOVE_PRUNING
        && !inCheck
        && m_nullmoveAllowed
        && eval >= beta  //very good score
        && depth >= NULLMOVE_REDUCTION_FACTOR + (depth / 5)  //enough depth
        && Evaluation::AreHeavyPieces(board)  //active player has pieces on the board (to avoid zugzwang in K+P endgames)
    ) {
        D( m_debug.Increment("NullMove Hits") );

        board.MakeNull();
        m_ply++;
        m_nullmoveAllowed = false;

        int R = NULLMOVE_REDUCTION_FACTOR + (depth / 5);
        int nullDepth = depth - R;
        int nullScore = -NegaMax(board, nullDepth, -beta, -beta + 1);

        board.TakeNull();
        m_ply--;
        m_nullmoveAllowed = true;

        if(nullScore >= beta) {
            Hash::tt.AddEntry(board.ZKey(), nullScore, TTENTRY_TYPE::LOWER_BOUND, Move(), nullDepth, m_ply, m_counter);
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
        extension++;
        D( m_debug.Increment("NegaMax Extension One-reply") );
    }

    // --------- Check for checkmate and stalemate -----------
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("NegaMax Checkmate") );
            return -MATESCORE + m_ply; //checkmate
        }
        else {
            D( m_debug.Increment("NegaMax Stalemate") );
            return 0; //stalemate
        }
    }

    // --------- Sort moves -----------
    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    // ------- Futility pruning --------
    //Prune quiet moves in the loop?
    bool doFutility = false;
    const int futilityMargin[3] = {0, 300, 500};
    if (depth <= 2 && !isPV && !inCheck && !IsMateValue(alpha) && !IsMateValue(beta)) {
        if(eval + futilityMargin[depth] < alpha) {
            doFutility = true;
            D( m_debug.Increment("NegaMax Futility (depth " + std::to_string(depth) + ")") );
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
        if(doFutility && move.Score() < 241) {
            if(eval + futilityMargin[depth] > bestScore)
                bestScore = eval + futilityMargin[depth];
            break;
        }

        // ---- Pawn to 7th/8th extension -----
        // if(move.PieceType() == PAWN
        //     && ColorlessRank(board.ActivePlayer(), move.ToSq()) >= RANK7
        // )
        //     extension++;

        // ----- Recapture extension ------
        if(isPV && !extension && move.MoveType() == CAPTURE
            && move.ToSq() == board.LastMove().ToSq()
            && move.CapturedType() == board.LastMove().CapturedType()
        ) {
            localExtension++;
            D( m_debug.Increment("NegaMax Extension Recapture") );
        }

        board.MakeMove(move);
        m_ply++; m_nodes++;

        //-------- Late Move Reductions ----------
        if( !TURNOFF_LMR
              && m_ply >= 3
              && depth >= 2         //avoid negative depths
              && !extension && !localExtension     //no extensions (including not in check)
              && moves.size() >= 6
        ) {
            if(move.Score() < 30) //uninteresting move
                reduction++;
            if(move.Score() >= 181 && move.Score() <= 187)
                reduction++;
            D( m_debug.Increment("NegaMax Late Move Reductions") );
        }

        // -------- Principal Variation Search -----------
        int extendedDepth = depth - 1 + extension + localExtension;
        int reducedDepth = extendedDepth - reduction;
        if(moveNumber == 1) {
            score = -NegaMax(board, extendedDepth, -beta, -alpha); //full window, no reduction
        }
        else {
            score = -NegaMax(board, reducedDepth, -alpha-1, -alpha); //zero window, reduction
            if(score > alpha && score < beta) { //'score < beta' is needed in fail-soft schemes
                D( m_debug.Increment("NegaMax PVS Out of Window") );
                score = -NegaMax(board, extendedDepth, -beta, -alpha);
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

                Hash::tt.AddEntry(board.ZKey(), score, TTENTRY_TYPE::LOWER_BOUND, move, depth, m_ply, m_counter);

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
        Hash::tt.AddEntry(board.ZKey(), bestScore, type, bestMove, depth, m_ply, m_counter);
    }

    return bestScore;
}

int Search::QuiescenceSearch(Board &board, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);

    D( m_debug.Increment("Quiescence Hits") );

    int bestScore = -INFINITE_SCORE;

    bool inCheck = board.IsCheck();

    //--------- Standpat -----------
    int standPat;
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
    if(ttEntry) {
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
    MoveList moves = gen.GenerateMoves(board);

    D( m_debug.Increment("Quiescence GenerateMoves") );

    //--------- Check for checkmate/stalemate ---------
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("Quiescence Checkmate") );
            return -MATESCORE + m_ply; //checkmate
        }
        else {
            D( m_debug.Increment("Quiescence Stalemate") );
            return 0; //stalemate
        }
    }

    //Order captures
    if(!inCheck)
        SortCaptures(board, moves);
    else
        SortEvasions(board, moves);

    for(auto move : moves) {

        if(!inCheck) {
            //Skip quiet moves (keep captures, promotions and enpassants)
            if(move.IsQuiet())
                continue;

            //Delta pruning
            const int SEE_EQUAL = 127;
            const int deltaMargin = 100;
            bool isCapture = move.MoveType() == CAPTURE;

            int standPatMargin = standPat + deltaMargin;
            if(isCapture && move.Score() == SEE_EQUAL && standPatMargin < alpha) {
                if(standPatMargin > bestScore)
                    bestScore = standPatMargin;
                continue;
            }

            //Skip negative SEE captures
            if(isCapture && move.Score() < SEE_EQUAL)
                continue;
        }
        
        D( Board bef = board );

        board.MakeMove(move);
        m_ply++; m_nodes++; m_selPly = std::max(m_selPly, m_ply);
        D( m_debug.Increment("Quiescence Nodes") );
        int score = -QuiescenceSearch(board, -beta, -alpha);
        board.TakeMove(move);
        m_ply--;

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
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = INFINITE;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    if(limits.infinite)   { Infinite(); return; }
    if(limits.depth)      { FixDepth(limits.depth); return; }
    if(limits.moveTime)   { FixTime(limits.moveTime); return; }
    if(limits.nodes)      { m_forcedNodes = limits.nodes; return; }
    if(!limits.movesToGo) { limits.movesToGo = 20; } //estimation of the remaining moves

    COLOR color = board.ActivePlayer();

    int myTime = (color == WHITE) ? limits.wtime : limits.btime;
    int yourTime = (color == WHITE) ? limits.btime : limits.wtime;

    int myInc = (color == WHITE) ? limits.winc : limits.binc;

    m_allocatedTime = myTime / limits.movesToGo + myInc;

    if(m_debugMode)
        std::cout << "info string TimeAllocation " <<  myTime << " " << yourTime << " " << m_allocatedTime << std::endl;
}

void Search::ProbeBoard(Board& board) {
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    for(auto move : moves)  {
        board.MakeMove(move);
        TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), 0);
        if(ttEntry) {
            P(move.Notation() << " " << ttEntry->type << "\t" << ttEntry->score);
        }
        board.TakeMove(move);
    }
}

void Search::Debug::Transform() {
    std::string colorGreen = "\033[1;92m";
    std::string colorBlack = "\033[0m";
    debugVariables[colorGreen+"NegaMax Calls to Evaluation (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax Calls to Evaluation"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"NegaMax Cutoffs (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax Cutoffs (score >= beta)"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"NegaMax GenerateMoves (permil)"+colorBlack] = 1000 * (double)debugVariables["NegaMax GenerateMoves Hits"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"TT Hits (in NegaMax) (permil)"+colorBlack] = 1000 * (double)debugVariables["TT Hits (in NegaMax)"] / debugVariables["NegaMax Hits"];
    debugVariables[colorGreen+"TT Hits (in Quiescence) (permil)"+colorBlack] = 1000 * (double)debugVariables["TT Hits (in Quiescence)"] / debugVariables["Quiescence Hits"];
}

void Search::Debug::Print() {
    Transform();
    for(auto variable : debugVariables) {
        P("\t " << variable.first << " " << variable.second);
    }
}
