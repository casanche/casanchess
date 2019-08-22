#include "Search.h"

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Sorting.h"
using namespace Sorting;

#include <algorithm> //max()
#include <cmath> //INFINITY
#include <iomanip> //debug output

const bool UCI_VERBOSE = false;

const bool TURNOFF_ASPIRATION_WINDOW = false;
const int ASPIRATION_WINDOW_DEPTH = 4;
const int ASPIRATION_WINDOW = 25;

const bool TURNOFF_NULLMOVE_PRUNING = false;
const int NULLMOVE_REDUCTION_FACTOR = 2;

const bool TURNOFF_LMR = false;
const bool TURNOFF_FUTILITY = false;

const int STATIC_MARGIN[4] = {0, 150, 300, 500};

// #define SEARCH_VERBOSE
// #define DEBUG_SEARCH
#ifdef DEBUG_SEARCH
    #define DEBUG_QUISCENCE
    #define DEBUG_NEGAMAX
    #define DEBUG_ROOTMAX
    #define DEBUG_ITERDEEP
#endif

Search::Search() {
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = 3500;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    m_bestScore = -INFINITE_SCORE;

    m_counter = 0;

    ClearSearch();
    Hash::tt.Clear();
    Hash::pawnHash.Clear();
}

Search::Search(Board board) : m_initialBoard(board) {
    Search();
}

void Search::ClearSearch() {
    m_elapsedTime = 0;
    m_nodes = 0;
    m_nps = 0;

    m_stop = false;
    m_nodesTimeCheck = 0;

    m_ply = 0;
    m_nullmoveAllowed = true;

    m_heuristics.killer.Clear();
    m_heuristics.history.Clear(); //or Age()
}

void Search::IterativeDeepening(Board &board) {
    m_initialBoard = board;
    ++m_counter;

    ClearSearch();

    m_startTime = Clock::now();

    for(m_depth = 1; m_depth <= m_maxDepth; m_depth++) {
        m_ply = 0;

        #ifdef DEBUG_ITERDEEP
        P("IterativeDeepening, currentDepth " << m_depth);
        #endif

        //Aspiration window
        int alpha, beta, score;
        bool aspiration = !TURNOFF_ASPIRATION_WINDOW && m_depth >= ASPIRATION_WINDOW_DEPTH;
        if(aspiration) {
            alpha = std::max(m_bestScore - ASPIRATION_WINDOW, -INFINITE_SCORE);
            beta  = std::min(m_bestScore + ASPIRATION_WINDOW,  INFINITE_SCORE);
        } else {
            alpha = -INFINITE_SCORE;
            beta  =  INFINITE_SCORE;
        }

        score = RootMax(m_initialBoard, m_depth, alpha, beta);
        
        //Out of aspiration bounds. Repeat search with infinite limits
        while(score <= alpha || score >= beta) {
            alpha = -INFINITE_SCORE;
            beta  = INFINITE_SCORE;
            score = RootMax(m_initialBoard, m_depth, alpha, beta);
        }

        //Filling variables
        m_elapsedTime = ElapsedTime();
        m_nps = static_cast<int>(1000 * m_nodes / (m_elapsedTime+1));

        //PV
        std::string PV = m_bestMove.Notation();
        bool showPV = 1;
        if(showPV) {
            Board newBoard = m_initialBoard;
            assert(newBoard == m_initialBoard);
            PV = "";
            for(int depth = 1; depth <= m_depth; ++depth) {
                TTEntry *ttEntry = Hash::tt.ProbeEntry(newBoard.ZKey(), 0);
                if(!ttEntry) continue;
                if(ttEntry->bestMove.MoveType() == NULLMOVE) break;

                Move bestMove = ttEntry->bestMove;
                PV += bestMove.Notation();
                PV += " ";

                newBoard.MakeMove(bestMove);
            }

            // Print move at depth i
            // int sign = (m_initialBoard.ActivePlayer() == WHITE) ? +1 : -1;
            // P( m_depth << "\t"<< CentipawnsToString(m_bestScore * sign) << "\t" << m_nodes << "\t" << m_elapsedTime << "\t" << PV );
        }

        if(m_stop)
             break;

        //Uci output
        UciOutput(PV);

        if(m_elapsedTime > (m_allocatedTime / 2) )
            break;

        #ifdef DEBUG
        ShowDebugInfo();
        #endif
    }

    std::cout << "bestmove " << m_bestMove.Notation() << std::endl;
}
void Search::UciOutput(std::string PV) {
    std::cout << "info depth " << m_depth;
    if(IsMateValue(m_bestScore)) {
        int mateScore = 0;
        if     (m_bestScore > 0) mateScore = INFINITE_SCORE - m_bestScore + 1; //ply+1
        else if(m_bestScore < 0) mateScore = -INFINITE_SCORE - m_bestScore - 1; //-ply-1
        std::cout << " score mate " << mateScore / 2; //return score in moves, not in plies
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

    #ifdef DEBUG_SEARCH
    P(" at RootMax, depth " << depth << " " << alpha << " " << beta);
    #endif
    #ifdef DEBUG
    debug_rootMax++;
    #endif

    // if(depth == 1) {
        // m_rootMoves.reserve(30);
        // MoveGenerator gen;
        // MoveList moves = gen.GenerateMoves(board);
        // for(auto move : moves) {
            // RootMove rootMove;
            // rootMove.move = move;
            // m_rootMoves.push_back(rootMove);
        // }
    // }
    // else {
        // SortRootMoves(m_rootMoves);
    // }
    
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    int count = 0;
    for(size_t i = 0; i < moves.size(); i++) {
    // for(auto &rootMove : m_rootMoves) {
        count++;

        // Move move = m_rootMoves[i].move;
        Move move = moves[i];

        // Move &move = rootMove.move;
        // m_currentRootMove = &m_rootMoves[i];

        // assert(move.MoveType());

        //Uci output
        if(m_elapsedTime > 1000) {
            std::cout << "info currmovenumber " << count;
            std::cout << " currmove " << move.Notation();
            std::cout << std::endl;
        }

        #ifdef DEBUG_ROOTMAX
        P(" RootmaxLoop, ply: " << m_ply << " move: " << move.Notation());
        // move.Print();
        #endif

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

        #ifdef SEARCH_VERBOSE
        P(i << " " << move.Notation() << " score: " << score);
        #endif
    }

    //Fill transposition tables
    bool outOfLimits = alpha <= alphaOrig || alpha >= beta;
    if(!m_stop && !outOfLimits) {
        m_bestMove = bestMove;
        m_bestScore = alpha;

        Hash::tt.AddEntry(board.ZKey(), m_bestScore, TTENTRY_TYPE::EXACT, m_bestMove, depth, m_counter);
    }

    return alpha;
}

int Search::NegaMax(Board &board, int depth, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);
    assert(depth >= 0);

    bool isPV = (beta - alpha) > 1;

    #ifdef DEBUG_NEGAMAX
    P("  at NegaMax, ply: " << m_ply << " alpha: " << alpha << " beta: " << beta);
    #endif

    #ifdef DEBUG
    debug_negaMax++;
    #endif

    // --------- Should I stop? -----------
    m_nodesTimeCheck++;
    if( m_stop || TimeOver() ) {
        m_stop = true;
        return 0;
    }

    // --------- Repetition draws and 50-move rule -----------
    if(board.IsRepetitionDraw(m_ply) || board.FiftyRule() >= 100) {
        #ifdef DEBUG
        debug_negaMaxRep++;
        #endif

        return 0;
    }

    //---------- Mate distance pruning ------------- //Different for PV nodes?
    alpha = std::max(alpha, -INFINITE_SCORE + m_ply);
    beta = std::min(beta, +INFINITE_SCORE - m_ply - 1);
    if(alpha >= beta) {
        #ifdef DEBUG
        debug_MDP++;
        #endif

        return alpha;
    }

    // --------- Check extension -----------
    int extension = 0;
    bool inCheck = board.IsCheck();
    if(inCheck)
        extension++;

    // --------- Quiescence search -----------
    if(depth + extension == 0 || m_futility) {
        return QuiescenceSearch(board, alpha, beta);
    }

    // --------- Transposition table probe --------
    Move bestMove; //for later storage in the Transposition Table
    int bestScore = -INFINITE_SCORE;
    int alphaOriginal = alpha; //for later calculation of TTENTRY_TYPE
    //ttEntry access
    TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), depth);
    if(ttEntry) {
        #ifdef DEBUG
        debug_tthits++;
        #endif

        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && ttEntry->score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && ttEntry->score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && ttEntry->score >= alpha && ttEntry->score <= beta) )
        {
            return ttEntry->score;
        }
    }

    //Calculate evaluation once at start
    int eval = Evaluation::Evaluate(board);

    // --- Static null-move pruning (aka Reverse futility) ---
    if(depth <= 3 && !isPV && !inCheck
        && Evaluation::AreHeavyPieces(board)
    ) {
        int evalMargin = eval - STATIC_MARGIN[depth];
        if(evalMargin >= beta)
            return evalMargin;
    }

    // ------- Futility pruning ---------
    // if( !TURNOFF_FUTILITY
    //     && depth == 1       //frontier (d=1) and pre-frontier (d=2)
    //     && !inCheck
    //     && board.LastMove().Score() == 0  //uninteresting move
    //     && !IsMateValue(alpha)
    // ) {
    //     int score = Evaluation::Evaluate(board);
    //     const int futilityMargin[2] = {250, 450};
    //     if( alpha > score + futilityMargin[0] )
    //         return QuiescenceSearch(board, alpha, beta);
    // }

    // --------- Razoring -----------
    // if(depth == 2 && !inCheck && !isPV && !m_nullmoveAllowed) {
    //     int score = Evaluation::Evaluate(board);
    //     if(score >= beta + RAZORING_DELTA)
    //         return score;
    // }

    // --------- Null-move pruning -----------
    if(!TURNOFF_NULLMOVE_PRUNING
        && eval >= beta  //very good score
        && m_nullmoveAllowed
        // && board.LastMove().MoveType != NULLMOVE
        && depth >= NULLMOVE_REDUCTION_FACTOR + 1 + (depth>=9)  //enough depth
        && !inCheck
        && Evaluation::AreHeavyPieces(board)  //active player has pieces on the board (to avoid zugzwang in K+P endgames)
    ) {
        #ifdef DEBUG
        debug_nullmove++;
        #endif

        board.MakeNull();
        m_ply++;
        m_nullmoveAllowed = false;

        int nullDepth = depth - 1 - NULLMOVE_REDUCTION_FACTOR - (depth>=9);
        int nullScore = -NegaMax(board, nullDepth, -beta, -beta + 1);

        board.TakeNull();
        m_ply--;
        m_nullmoveAllowed = true;

        if(nullScore >= beta) {
            Hash::tt.AddEntry(board.ZKey(), nullScore, TTENTRY_TYPE::LOWER_BOUND, Move(), nullDepth, m_counter);
            return nullScore;   //nullScore or beta
        }
    }
    //Allow non-consecutive null-move pruning
    m_nullmoveAllowed = true;

    // --------- Generate the moves -----------
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    #ifdef DEBUG
    debug_negaMax_generation++;
    #endif

    //----- One-reply extension -------
    if(moves.size() == 1)
        extension++;

    // --------- Check for checkmate and stalemate -----------
    if( moves.empty() ) {
        if(inCheck)
            return -INFINITE_SCORE + m_ply; //checkmate
        else
            return 0; //stalemate
    }

    // --------- Sort moves -----------
    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    int moveNumber = 0;
    for(auto move : moves) {
        moveNumber++;
        int score, reduction = 0;
        int localExtension = 0;

        assert(move.MoveType());

        #ifdef DEBUG_NEGAMAX
        P("  NegamaxLoop, ply " << m_ply << " move: " << move.Notation());
        #endif

        // ---- Pawn to 7th/8th extension -----
        // if(move.PieceType() == PAWN
        //     && ColorlessRank(board.ActivePlayer(), move.ToSq()) >= RANK7
        // )
        //     extension++;

        // ----- Recapture extension ------
        if(isPV && !extension && move.MoveType() == CAPTURE
            && move.ToSq() == board.LastMove().ToSq()
            && move.CapturedType() == board.LastMove().CapturedType()
        )
            localExtension++;

        board.MakeMove(move);
        m_ply++; m_nodes++;

        //-------- Late Move Reductions ----------
        if( !TURNOFF_LMR
              && m_ply >= 3
              && depth >= 2         //avoid negative depths
              && extension == 0     //not in check
              && moves.size() >= 6
              && move.Score() == 0  //uninteresting move
            //   && !board.IsCheck()
        ) {
            reduction++;
        }

        // ------- Futility pruning ---------
        // m_futility = false;
        // P( !IsMateValue(alpha) );
        // if( !TURNOFF_FUTILITY
        //     && reducedDepth == 1            //frontier (d=1) and pre-frontier (d=2)
        //     && !inCheck
        //     && move.Score() == 0                    //uninteresting move
        //     && !IsMateValue(alpha)
        //     && !board.IsCheck()
        // ) {
        //     // const int futilityMargin[2] = {250, 450};
        //     P("hi");
        //     if( alpha > 300 + Evaluation::Evaluate(board) )
        //         m_futility = true;
        //         // return QuiescenceSearch(board, alpha, beta);
        // }

        // -------- Principal Variation Search -----------
        int extendedDepth = depth - 1 + extension + localExtension;
        int reducedDepth = extendedDepth - reduction;
        if(moveNumber == 1) {
            score = -NegaMax(board, extendedDepth, -beta, -alpha); //full window, no reduction
        }
        else {
            score = -NegaMax(board, reducedDepth, -alpha-1, -alpha); //zero window, reduction
            if(score > alpha && score < beta) //'score < beta' is needed in fail-soft schemes
                score = -NegaMax(board, extendedDepth, -beta, -alpha);
        }

        board.TakeMove(move);
        m_ply--;

        if(score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        if(score > alpha) {

            if(score >= beta) {
                #ifdef DEBUG
                debug_negaMax_cutoffs++;
                #endif

                Hash::tt.AddEntry(board.ZKey(), score, TTENTRY_TYPE::LOWER_BOUND, move, depth, m_counter);

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

    TTENTRY_TYPE type = (alpha > alphaOriginal) ? TTENTRY_TYPE::EXACT : TTENTRY_TYPE::UPPER_BOUND;
    Hash::tt.AddEntry(board.ZKey(), alpha, type, bestMove, depth, m_counter); //alpha or bestScore?

    return alpha;
}

int Search::QuiescenceSearch(Board &board, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);

    #ifdef DEBUG_QUISCENCE
    P("    at Quiescence, ply: " << m_ply << " alpha: " << alpha << " beta: " << beta);
    #endif
    #ifdef DEBUG
    debug_quiescence++;
    #endif

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
    }
    
    // --------- Transposition table lookup -----------
    TTEntry* ttEntry = Hash::tt.ProbeEntry(board.ZKey(), 0);
    if(ttEntry) {
        #ifdef DEBUG
        debug_quiescence_tthits++;
        #endif

        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && ttEntry->score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && ttEntry->score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && ttEntry->score >= alpha && ttEntry->score <= beta) )
        {
            return ttEntry->score;
        }
    }

    //-------- Generate moves ----------
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);

    //--------- Check for checkmate/stalemate ---------
    if( moves.empty() ) {
        if(inCheck)
            return -INFINITE_SCORE + m_ply; //checkmate
        else
            return 0; //stalemate
    }

    //Order captures
    if(!inCheck)
        SortCaptureMoves(board, moves);

    for(auto move : moves) {

        if(!inCheck) {
            //Skip quiet moves (keep captures, promotions and enpassants)
            if(move.IsQuiet())
                continue;

            //Delta pruning
            const int SEE_EQUAL = 127;
            const int deltaMargin = 100;
            bool isCapture = move.MoveType() == CAPTURE;
            if(isCapture && move.Score() == SEE_EQUAL && standPat < (alpha - deltaMargin) ) {
                continue;
            }

            //Skip negative SEE captures
            if(isCapture && move.Score() < SEE_EQUAL)
                continue;
        }
        
        #ifdef DEBUG_QUISCENCE
        P("    at QuiescenceLoop, ply: " << m_ply << " alpha: " << alpha << " beta: " << beta << " move: " << move.Notation());
        #endif

        #ifdef DEBUG2
        Board bef = board;
        #endif

        board.MakeMove(move);
        m_ply++; m_nodes++;
        int score = -QuiescenceSearch(board, -beta, -alpha);
        board.TakeMove(move);
        m_ply--;

        #ifdef DEBUG2
        Board aft = board;
        assert(bef == aft);
        #endif

        if(score > alpha) {
            if(score >= beta)
                return score;
            alpha = score;
        }
    }

    return alpha;
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
    //estimation of the remaining moves
    if(!limits.movesToGo) { limits.movesToGo = 20; }

    COLORS color = board.ActivePlayer();

    int myTime = (color == WHITE) ? limits.wtime : limits.btime;
    int yourTime = (color == WHITE) ? limits.btime : limits.wtime;

    int myInc = (color == WHITE) ? limits.winc : limits.binc;
    // int yourInc = (color == WHITE) ? limits.binc : limits.winc;

    // double timeRatio = (double)(myTime+1) / (double)(yourTime+1);

    m_allocatedTime = myTime / limits.movesToGo + myInc;

    if(UCI_VERBOSE)
        std::cout << "info string AllocateTime " <<  myTime << " " << yourTime << " " << m_allocatedTime << std::endl;
}

void Search::ProbeBoard() {
    Board board = m_initialBoard;

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

void Search::ShowDebugInfo() {
    P("\t RootMax searches " << debug_rootMax);
    P("\t NegaMax searches " << debug_negaMax);
    P("\t Null-move searches " << debug_nullmove);
    P("\t Quiescence searches " << debug_quiescence);
    P("\t TT hits " << debug_tthits << " / " << debug_negaMax \
        << " (" << std::setprecision(1) << std::fixed << (float)debug_tthits / debug_negaMax * 100 << "%)");
    P("\t TT hits (QS) " << debug_quiescence_tthits << " / " << debug_quiescence \
        << " (" << std::setprecision(1) << std::fixed << (float)debug_quiescence_tthits / debug_quiescence * 100 << "%)");
    P("\t Repetition hits " << debug_negaMaxRep << " / " << debug_negaMax);
    P("\t Mate distance pruning " << debug_MDP << " / " << debug_negaMax);
    P("\t NegaMax move generation " << debug_negaMax_generation << " / " << debug_negaMax \
        << " (" << std::setprecision(1) << std::fixed << (float)debug_negaMax_generation / debug_negaMax * 100 << "%)");
    P("\t NegaMax beta cutoffs " << debug_negaMax_cutoffs << " / " << debug_negaMax \
        << " (" << std::setprecision(1) << std::fixed << (float)debug_negaMax_cutoffs / debug_negaMax * 100 << "%)");
    P("\t TT fill " << Hash::tt.NumEntries() << " / " << Hash::tt.Size() \
        << " (" << std::setprecision(1) << std::fixed << (float)Hash::tt.NumEntries() / Hash::tt.Size() * 100 << "%)");
}
