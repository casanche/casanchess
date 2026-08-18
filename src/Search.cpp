// Search.cpp
//
// Using fail-soft variant of alpha-beta
//
// - Main search algorithms:
//    - Iterative deepening: initial loop of increasing depth
//    - RootMax: special search for the root node
//    - NegaMax: main recursive alpha-beta search
//    - QuiescenceSearch: search in leaf nodes to avoid horizon effect
//
// - Alpha-beta improvements:
//    - Aspiration Window: narrow alpha-beta bounds around expected score
//    - Principal Variation Search (PVS): narrow alpha-beta bounds for efficient PV search
//
// - Pruning:
//    - Null-move pruning: quick pruning of clearly winning positions
//    - Futility Pruning: pruning of non-promising moves
//    - Reverse futility pruning: early pruning using static evaluation
//
// - Extensions:
//    - Check extension
//    - One-reply extension
//    - Recapture extension
//
// - Reductions:
//    - Late Move Reductions: reduce search depth for late non-promising moves
//
// - Heuristics:
//    - History heuristics: reward successful moves in past positions
//    - Killer moves
//    - Transposition tables
//    - Time management


#include "Search.h"

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "MoveScorer.h"
#include "Syzygy.h"
#include "Uci.h"
using namespace Sorting;

#include <algorithm> //max(), clamp()
#include <cmath> //INFINITY
#include <iomanip> //debug output

// Search parameters
const int MAX_QS_PLIES = 128;  // Maximum depth limit for quiescence search

// Aspiration window parameters - used to narrow alpha-beta bounds around expected score
const bool TURNON_ASPIRATION_WINDOW = true;
const int ASPIRATION_WINDOW_DEPTH = 4;         // Minimum depth to enable aspiration window
const int ASPIRATION_WINDOW = 25;              // Initial half-window size in centipawns
const int ASPIRATION_WINDOW_MULTIPLIER = 2;    // Incremental window resize

const bool TURNOFF_NULLMOVE_PRUNING = false;
const int NULLMOVE_REDUCTION_FACTOR = 3;

const bool TURNOFF_LMR = false;
const bool TURNOFF_FUTILITY = false;

// Draw contempt to discourage premature draws
constexpr int DrawScore(int ply) {
    return 0;
    // return (ply & 1) ? 10 : -10;
}

// Called once when the UCI interface starts up.
Search::Search(TT& tt): m_tt(tt) {
    ClearSearch(true);

    m_searchCount = 0;
}

// Reset state for a new position search.
// Called at the beginning of each iteration in Iterative Deepening.
void Search::ClearSearch(bool fullClear) {
    // Node counters
    m_nodes = 0;
    m_tbHits = 0;

    // Search state
    m_pv.ClearTable();
    m_bestScore = NO_SCORE;
    m_bestMove = Move();

    // Depth tracking
    m_ply = 0;
    m_plyqs  = 0;
    m_selPly = 0;

    // Pruning flags
    m_nullmoveAllowed = true;

    // Move ordering
    m_heuristics.killer.Clear();
    m_heuristics.history.Age(); // Reduce history from old positions, but do not remove entirely

    // Debug
    m_debug.Clear();

    // Completely clear hashes and history heuristics for a reproducible search
    if(fullClear) {
        assert(m_tt.Occupancy() == 0);
        m_evalCache.Clear();
        Hash::pawnHash.Clear();

        m_heuristics.history.Clear();
    }
}

// Main loop: increase depth one by one and call the root search.
// Manage time, aspiration window, and UCI output.
void Search::IterativeDeepening(Board &board, const UCI_Limits& limits, bool fullClear) {
    ClearSearch(fullClear);

    MoveList rootMoves = MoveGenerator::GenerateMoves(board);
    size_t movesSize = rootMoves.size();

    m_limits.StartNewSearch(board.ActivePlayer(), limits, movesSize);
    
    D( m_debug.Increment("IterativeDeepening: _: Start") );
    m_searchCount++;

    for(m_depth = 1; m_depth <= m_limits.MaxDepth(); m_depth++) {
        assert(m_ply == 0);
        assert(m_plyqs == 0);
        assert(m_nullmoveAllowed);

        m_selPly = 0;

        AspirationWindow(board, m_depth, m_bestScore);

        // Stop search if limits are reached within the loop
        if(m_limits.Stopped()) break;

        i64 elapsedTime = m_limits.UpdatedElapsedTime();
        Uci::Output(m_depth, m_selPly, m_bestScore, m_nodes, elapsedTime, m_limits.CalculateNPS(m_nodes), m_tbHits, BOUND_TYPE::EXACT, m_pv.PVString(), m_tt);

        // Stop search if we used half of the allocated time, since
        // next iteration will likely use more than the allocated time
        if(elapsedTime > (m_limits.AllocatedTime() / 2))
            break;

        if(DEBUG_PRINT_STATISTICS && m_depth == m_limits.MaxDepth())
            D( m_debug.Print() );
    }

    m_limits.WaitIfNecessary();

    Uci::BestMove(m_bestMove.Notation(), m_pv.PonderString());
}

// Narrow alpha-beta bounds around expected score
int Search::AspirationWindow(Board& board, const int depth, const int bestScore) {
    const bool aspiration = TURNON_ASPIRATION_WINDOW && depth >= ASPIRATION_WINDOW_DEPTH && !IsWinValue(bestScore);
    if(!aspiration)
        return RootMax(board, depth, -INFINITE_SCORE, INFINITE_SCORE);

    int window = ASPIRATION_WINDOW;
    int alpha = bestScore - window;
    int beta  = bestScore + window;

    int score = RootMax(board, depth, alpha, beta);

    for(int researches = 1; !m_limits.Stopped() && (score <= alpha || score >= beta); researches++) {
        D( m_debug.Increment("AspirationWindow: Out of bounds: Researches: " + std::to_string(researches) ); );

        // Display lowerbound / upperbound info
        BOUND_TYPE bound = (score <= alpha) ? BOUND_TYPE::UPPER_BOUND
                                            : BOUND_TYPE::LOWER_BOUND;
        i64 elapsedTime = m_limits.UpdatedElapsedTime();
        Uci::Output(m_depth, m_selPly, score, m_nodes, elapsedTime, m_limits.CalculateNPS(m_nodes), m_tbHits, bound, m_pv.PVString(), m_tt);

        // Asymmetrical incremental aspiration
        window = window * ASPIRATION_WINDOW_MULTIPLIER;
        if(score <= alpha) {
            alpha = bestScore - window;
        } else if(score >= beta) {
            beta = bestScore + window;
        }

        if(researches == 4 || IsWinValue(score)) {
            alpha = -INFINITE_SCORE;
            beta = INFINITE_SCORE;
        }

        score = RootMax(board, depth, alpha, beta);
    }

    return score;
}

// Root search. Handle special root-level logic.
// Iterate over all moves, call NegaMax, and update the best move found.
int Search::RootMax(Board &board, int depth, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(depth > 0);

    D( m_debug.Increment("RootMax: _: Hit") );

    m_nodes++;

    Move bestMove;
    int bestScore = NO_SCORE;
    int alphaOriginal = alpha; // Save the original alpha for TT logic
    int score;
    i64 elapsedTime;

    m_pv.ClearPly(m_ply);

    MoveList moves = MoveGenerator::GenerateMoves(board);
    D( if(depth == 1) P("Number of moves in root position: " << moves.size()) );

    Move hashMove; // For move ordering
    TTEntry* ttEntry = m_tt.Probe(board.ZKey());
    if(ttEntry)
        hashMove = ttEntry->bestMove;

    SortMoves(board, moves, hashMove, m_heuristics, m_ply);

    if(!m_bestMove.MoveType() && !moves.empty())
        m_bestMove = moves[0]; // Safety net if first move at depth 1 is not completed

    int moveNumber = 0;

    for(auto move : moves) {
        moveNumber++;
        bool isPV = (moveNumber == 1);

        if(DEBUG_SEARCH_TREE)
            P( "RootMax: " << move.Notation() );

        // Display the root move under analysis and update the counters
        elapsedTime = m_limits.UpdatedElapsedTime();
        Uci::RootUpdate(m_depth, m_selPly, moveNumber, move.Notation(), m_nodes, elapsedTime, m_limits.CalculateNPS(m_nodes), m_tbHits);

        board.MakeMove(move);
        m_ply++;

        // -------- Principal Variation Search (PVS) -----------
        // PV move: full window
        // Other moves: zero window
        if(isPV)
            score = -NegaMax(board, depth-1, -beta, -alpha);
        else {
            score = -NegaMax(board, depth-1, -alpha-1, -alpha);

            // Score within window: new PV found! Re-search with full window
            if(score > alpha && score < beta)
                score = -NegaMax(board, depth-1, -beta, -alpha);
        }

        board.TakeMove(move);
        m_ply--;

        if(m_limits.Stopped()) break;

        if(score > bestScore) {
            D( m_debug.Increment("RootMax: AlphaBeta: Update BestMove (score > bestScore)") );
            bestScore = score;
            bestMove = move;
        }

        // Not useful to store in TT due to aspiration window
        if(score >= beta) {
            D( m_debug.Increment("RootMax: AlphaBeta: Beta Cutoff (score >= beta)") );
            m_bestMove = move;

            break;
        }

        if(score > alpha) {
            D( m_debug.Increment("RootMax: AlphaBeta: Update Alpha (score > alpha)") );
            alpha = score;

            m_bestMove = move;
            m_bestScore = score;

            m_pv.Update(m_ply, move);

            elapsedTime = m_limits.ElapsedTime();
            if(elapsedTime > UCI_OUTPUT_ROOTMAX_MINTIME) {
                elapsedTime = m_limits.UpdatedElapsedTime();
                Uci::Output(m_depth, m_selPly, score, m_nodes, elapsedTime, m_limits.CalculateNPS(m_nodes), m_tbHits, BOUND_TYPE::LOWER_BOUND, m_pv.PVString(), m_tt);
            }
        }
    }

    // Store "exact" score in transposition table if search finished within search bounds
    bool withinBounds = (bestScore > alphaOriginal) && (bestScore < beta);
    if(!m_limits.Stopped() && withinBounds) {
        assert(bestMove == m_bestMove && bestScore == m_bestScore);
        D( m_debug.Increment("RootMax: AlphaBeta: TT Store Exact") );

        m_tt.Store(board.ZKey(), bestScore, TTENTRY_TYPE::EXACT, bestMove, depth, m_ply, m_searchCount);
    }

    return bestScore;
}

// Negamax search: core recursive alpha-beta search.
// Implements most of the engine's search logic.
int Search::NegaMax(Board &board, int depth, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);

    D( m_debug.Increment("NegaMax: _: Entering function") );

    m_nodes++;

    //---------- Safety net -------------
    // Prevents the search from going too deep and crashing the engine
    if(m_ply >= MAX_PLY-1) {
        D( m_debug.Increment("NegaMax: Safety: MAX_PLY reached") );
        return Evaluation::Evaluate(board);
    }

    const bool isPV = (beta - alpha) != 1;
    if(isPV)
        m_pv.ClearPly(m_ply);

    // --------- Termination checks -----------
    if(m_limits.LimitsReached(m_nodes))
        return 0;

    // --------- Draw detection: repetition and 50-move rule -----------
    if(board.FiftyRule() >= 100) {
        D( m_debug.Increment("NegaMax: Draw: FiftyRule") );
        return DrawScore(m_ply);
    }
    if(board.IsRepetitionDraw()) {
        D( m_debug.Increment("NegaMax: Draw: Repetition") );
        return DrawScore(m_ply);
    }

    //---------- Mate distance pruning -------------
    // Prevents the search from reporting mates that are too far away
    alpha = std::max(alpha, -MATESCORE_MAX + m_ply);
    beta = std::min(beta, +MATESCORE_MAX - m_ply - 1);
    if(alpha >= beta) {
        D( m_debug.Increment("NegaMax: Pruning: MateDistance") );
        return alpha;
    }

    // --------- Check extension -----------
    // Extend search if in check
    int extension = 0;
    bool inCheck = board.IsCheck();
    if(inCheck) {
        extension++;
        D( m_debug.Increment("NegaMax: Extension: Check") );
    }

    // --------- Quiescence search -----------
    // Activated at leaf nodes
    if(depth <= 0 && !inCheck) {
        return QuiescenceSearch(board, alpha, beta);
    }

    D( m_debug.Increment("NegaMax: _: Hits") );

    // --------- Transposition table probe --------
    Move bestMove; // For later storage in the TT
    int bestScore = NO_SCORE;
    int alphaOriginal = alpha; // For TT entry type calculation

    Move hashMove; // For move ordering
    int ttEval = NO_EVAL;
    
    TTEntry* ttEntry = m_tt.Probe(board.ZKey());
    if(ttEntry) {
        D( m_debug.Increment("NegaMax: TT: Hit") );
        ttEval = ttEntry->eval;
        hashMove = ttEntry->bestMove;

        if(!isPV && ttEntry->depth >= depth) {
            D( m_debug.Increment("NegaMax: TT: Higher Depth") );
            int score = m_tt.ScoreFromHash(ttEntry->score, m_ply);

            if( ttEntry->type == TTENTRY_TYPE::EXACT
                || (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
                || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            ) {
                D( m_debug.Increment("NegaMax: TT: Cut-Off") );
                return score;
            }
        }
    }

    // --------- Syzygy endgame probe ---------
    Syzygy::TB_RESULT tb_result;
    if( Syzygy::Probe_WDL(board, tb_result) ) {
        m_tbHits++;

        int score;
        TTENTRY_TYPE bound;

        if(tb_result == Syzygy::TB_RESULT::WIN) {
            score = TBWIN - m_ply;
            bound = TTENTRY_TYPE::LOWER_BOUND;
        } else if(tb_result == Syzygy::TB_RESULT::LOSS) {
            score = -TBWIN + m_ply;
            bound = TTENTRY_TYPE::UPPER_BOUND;
        } else {
            score = DrawScore(m_ply);
            bound = TTENTRY_TYPE::EXACT;
        }

        // Check if score is a cut-off
        if( (score >= beta  && bound != TTENTRY_TYPE::UPPER_BOUND)
         || (score <= alpha && bound != TTENTRY_TYPE::LOWER_BOUND) )
        {
            m_tt.Store(board.ZKey(), score, bound, Move(), MAX_DEPTH, m_ply, m_searchCount);
            return score;
        }

        // If not, update variables
        if(bound != TTENTRY_TYPE::UPPER_BOUND) {
            if(score > bestScore)
                bestScore = score;
            if(score > alpha)
                alpha = score;
        }
    }

    // --------- Static Evaluation ---------
    // Used in pruning heuristics
    int eval = NO_EVAL;
    if(!inCheck) {
        D( m_debug.Increment("NegaMax: Evaluation: 1: Enter") );
        if(ttEval != NO_EVAL) {
            D( m_debug.Increment("NegaMax: Evaluation: 2.1: Use TT Eval") );
            eval = ttEval;
        } else if(m_evalCache.Probe(board.ZKey(), eval)) {
            D( m_debug.Increment("NegaMax: Evaluation: 2.2: Use EvalCache") );
        } else {
            D( m_debug.Increment("NegaMax: Evaluation: 3: Call Evaluate()") );
            eval = Evaluation::Evaluate(board);
            m_evalCache.Store(board.ZKey(), eval);
        }
    }

    // --- Reverse Futility Pruning ---
    // Prune if static evaluation is too good (eval >> beta)
    const int staticMargin = 100;
    if(depth <= 4 && !isPV && !inCheck) {
        int staticEval = eval - depth * staticMargin;
        if(staticEval >= beta) {
            D( m_debug.Increment("NegaMax: Pruning: Reverse Futility") );
            D( m_debug.Increment("NegaMax: Pruning: Reverse Futility - Depth " + std::to_string(depth)) );
            return staticEval;
        }
    }

    // --------- Null-move pruning -----------
    // Skip a move (null move) to quickly detect beta cutoffs
    if(!TURNOFF_NULLMOVE_PRUNING
        && !isPV
        && !inCheck
        && m_nullmoveAllowed
        && eval >= beta  //very good score
        && depth > 1
        // && depth >= NULLMOVE_REDUCTION_FACTOR + (depth / 5)  //enough depth
        && Evaluation::AreHeavyPieces(board)  // Avoid zugzwang in K+P endgames
    ) {
        D( m_debug.Increment("NegaMax: Pruning: NullMove: Hit") );
        D( m_debug.Increment("NegaMax: Pruning: NullMove: Hit - Depth " + std::to_string(depth)) );

        board.MakeNull();
        m_ply++;
        m_nullmoveAllowed = false;

        int R = NULLMOVE_REDUCTION_FACTOR + (depth / 4);
        int nullDepth = std::max(0, depth - R);
        int nullScore = -NegaMax(board, nullDepth, -beta, -beta + 1);

        board.TakeNull();
        m_ply--;
        m_nullmoveAllowed = true;

        if(nullScore >= beta) {
            D( m_debug.Increment("NegaMax: Pruning: NullMove: Beta Cutoff") );
            D( m_debug.Increment("NegaMax: Pruning: NullMove: Beta Cutoff - Depth " + std::to_string(depth)) );
            if(IsWinValue(nullScore))
                nullScore = beta;  // Avoid reporting false mates in zugzwang
            m_tt.Store(board.ZKey(), nullScore, TTENTRY_TYPE::LOWER_BOUND, Move(), nullDepth, m_ply, m_searchCount, eval);
            return nullScore;
        }
    }
    // Allow non-consecutive null-move pruning
    m_nullmoveAllowed = true;

    // --------- Move generation -----------
    MoveList moves = MoveGenerator::GenerateMoves(board);

    D( m_debug.Increment("NegaMax: _: GenerateMoves") );

    // --------- Check for checkmate and stalemate -----------
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("NegaMax: EmptyMoves: Checkmate") );
            return -MATESCORE_MAX + m_ply; // Checkmate
        }
        else {
            D( m_debug.Increment("NegaMax: EmptyMoves: Stalemate") );
            return DrawScore(m_ply); // Stalemate
        }
    }

    //----- One-reply extension -------
    if( moves.size() == 1 ) {
        D( m_debug.Increment("NegaMax: Extension: One-reply") );
        extension++;
    }

    // --------- Move ordering ---------
    // Order moves to maximize search efficiency (hash move, captures, killers, history...)
    SortMoves(board, moves, hashMove, m_heuristics, m_ply);

    // --------- Move loop ---------
    int moveNumber = 0;

    for(auto move : moves) {
        assert(move.MoveType());

        moveNumber++;
        bool childPV = (moveNumber == 1);

        int score, reduction = 0;
        int localExtension = 0;

        if(DEBUG_SEARCH_TREE)
            P("  NegamaxLoop, ply " << m_ply << " move: " << move.Notation());

        // ------- Futility pruning -------
        // Prune quiet moves and bad captures if unlikely to raise alpha
        const int futilityMargin = 0 + depth * 25;
        if(!TURNOFF_FUTILITY && !isPV && !childPV && !inCheck && !IsWinValue(alpha)
            && depth <= 4
            && eval + futilityMargin <= alpha
            && ( move.Score() < 120 || (move.Score() >= 181 && move.Score() <= 188) )
        ) {
            D( m_debug.Increment("NegaMax: Pruning: Futility") );
            D( m_debug.Increment("NegaMax: Pruning: Futility - Depth " + std::to_string(depth)) );
            if(eval + futilityMargin > bestScore) {
                bestScore = eval + futilityMargin;
            }
            continue;
        }

        // ----- Recapture extension ------
        // Extend if the move is a recapture of the same piece type
        if(!extension
            && move.MoveType() == CAPTURE
            && board.LastMove().MoveType() == CAPTURE
            && move.ToSq() == board.LastMove().ToSq()
        ) {
            D( m_debug.Increment("NegaMax: Extension: Recapture") );
            localExtension++;
        }

        // -------- Late Move Reductions ----------
        // Reduce depth of less-promising moves
        if( !TURNOFF_LMR
              && moveNumber > 1     // Never reduce the first move
              && depth >= 2         // Avoid negative depths
              && !inCheck           // Not in check
        ) {
            reduction = LateMoveReductions((int)move.Score(), depth, moveNumber, isPV);
        }

        board.MakeMove(move);
        m_ply++;

        // -------- Principal Variation Search (PVS) -----------
        int fullDepth = depth - 1 + extension + localExtension;
        int reducedDepth = std::max(0, fullDepth - reduction);

        // PV move: full window, full depth
        // Other moves: zero window, reduced depth
        if(isPV && childPV)
            score = -NegaMax(board, fullDepth, -beta, -alpha);
        else {
            score = -NegaMax(board, reducedDepth, -alpha-1, -alpha);

            // Reduced search failed high: re-search with full depth
            if(reduction && score > alpha)
                score = -NegaMax(board, fullDepth, -alpha-1, -alpha);

            // Score within window: new PV found! Re-search with full window and depth
            if(score > alpha && score < beta) // 'score < beta' needed in fail-soft schemes
                score = -NegaMax(board, fullDepth, -beta, -alpha);
        }

        board.TakeMove(move);
        m_ply--;

        if(m_limits.Stopped()) return 0;

        if(score > bestScore) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Update BestMove (score > bestScore)") );
            bestScore = score;
            bestMove = move;
        }

        if(score >= beta) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Beta Cutoff (score >= beta)") );

            m_tt.Store(board.ZKey(), score, TTENTRY_TYPE::LOWER_BOUND, move, depth, m_ply, m_searchCount, eval);

            // Update heuristics
            if( move.IsQuiet() ) {
                m_heuristics.killer.Update(move, m_ply);
                m_heuristics.history.GoodHistory(move, board.ActivePlayer(), depth);
            }

            return score;
        }

        if(score > alpha) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Update Alpha (score > alpha)") );
            alpha = score;
 
            m_pv.Update(m_ply, move);
        }

    } // End move loop

    // Store score in transposition table
    if(bestMove.MoveType() != 0) {
        bool withinBounds = (bestScore > alphaOriginal);
        D( m_debug.Increment("NegaMax: AlphaBeta: " + std::string(withinBounds ? "Exact" : "UpperBound")) );
        TTENTRY_TYPE type = withinBounds ? TTENTRY_TYPE::EXACT
                                         : TTENTRY_TYPE::UPPER_BOUND;
        m_tt.Store(board.ZKey(), bestScore, type, bestMove, depth, m_ply, m_searchCount, eval);
    }

    return bestScore;
}

// Quiescence search: extends the search at the leaf nodes to avoid the horizon effect
// in tactical sequences (captures and checks).
int Search::QuiescenceSearch(Board &board, int alpha, int beta) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply < MAX_PLY);

    D( m_debug.Increment("Quiescence: _: Hits"); );
    D( if(m_plyqs <= 2 || m_plyqs % 5 == 0) m_debug.Increment("Quiescence: QPly " + std::to_string(m_plyqs)) );

    m_nodes++;

    // Termination checks
    if(m_limits.LimitsReached(m_nodes))
        return 0;

    // Prevent infinite recursion in rare cases (unlikely to occur)
    if (m_plyqs >= MAX_QS_PLIES) {
        D( m_debug.Increment("Quiescence: MAX_QS_PLIES reached (unlikely)") );
        return Evaluation::Evaluate(board);
    }

    const bool isPV = (beta - alpha) != 1;
    Move hashMove; // For move ordering
    int ttEval = NO_EVAL;

    // Probe transposition table.
    // Only non-PV nodes: PV nodes require the most accurate score possible.
    TTEntry* ttEntry = m_tt.Probe(board.ZKey());
    if(ttEntry) {
        D( m_debug.Increment("Quiescence: TT: Hit") );
        hashMove = ttEntry->bestMove;
        ttEval = ttEntry->eval;

        if(!isPV) {
            D( m_debug.Increment("Quiescence: TT: !isPV") );
            int score = m_tt.ScoreFromHash(ttEntry->score, m_ply);
            if( ttEntry->type == TTENTRY_TYPE::EXACT
                || (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
                || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            ) {
                D( m_debug.Increment("Quiescence: TT: Cut-Off") );
                return score;
            }
        }
    }

    bool inCheck = board.IsCheck();
    int bestScore = NO_SCORE;

    // Stand pat: static evaluation of current position
    // Probe evaluation cache first (modifies standPat if hit)
    int standPat = 0;
    if(!inCheck) {
        D( m_debug.Increment("Quiescence: Evaluation: 1: Enter") );
        if(ttEval != NO_EVAL) {
            D( m_debug.Increment("Quiescence: Evaluation: 2.1: Use TT Eval") );
            standPat = ttEval;
        } else if(m_evalCache.Probe(board.ZKey(), standPat)) {
            D( m_debug.Increment("Quiescence: Evaluation: 2.2: Use EvalCache") );
        } else {
            D( m_debug.Increment("Quiescence: Evaluation: 3: Call Evaluate()") );
            standPat = Evaluation::Evaluate(board);
            m_evalCache.Store(board.ZKey(), standPat);
        }

        if(standPat > alpha) {
            if(standPat >= beta) {
                D( m_debug.Increment("Quiescence: StandPat Cutoff (>= beta)") );
                return standPat;
            }
            alpha = standPat;
        }
        bestScore =  standPat;
    }

    // Generate captures.
    // If in check, generate check evasions.
    D( m_debug.Increment("Quiescence: GenerateMoves") );
    MoveList moves = inCheck ? MoveGenerator::GenerateEvasionMoves(board)
                             : MoveGenerator::GenerateTacticalMoves(board);

    // Checkmate (ignore stalemate)
    if( moves.empty() && inCheck ) {
        D( m_debug.Increment("Quiescence: EmptyMoves: Checkmate") );
        return -MATESCORE_MAX + m_ply;
    }

    // Different move ordering for efficiency
    inCheck ? SortEvasions(board, moves, hashMove)
            : SortTactical(board, moves, hashMove);

    for(auto move : moves) {

        if(!inCheck && move.IsCapture() && !move.IsPromotion()) {
            const int seeValue = (move == hashMove) ? board.SEE(move)
                                                    : Scorer::SEEFromTacticalScore( move.Score() );

            // Prune negative SEE captures
            if(seeValue < 0) {
                D( m_debug.Increment("Quiescence: Pruning SEE < 0") );
                continue;
            }
            
            // --- Delta Pruning ---
            // For SEE >= 0 captures
            // Use an optimistic score using the maximum potential gain to represent the move impact.
            const int DELTA_MARGIN_SAFE = 90;
            int estimatedScore = standPat + DELTA_MARGIN_SAFE + SEE::MATERIAL_VALUES[move.CapturedType()];

            if(estimatedScore < alpha) {
                D( m_debug.Increment("Quiescence: Pruning SEE >= 0: estimatedScore < alpha") );
                continue;
            }
        }
        
        D( BoardIdentity bef = BoardIntegrityChecker::GenerateBoardIdentity(board); );

        board.MakeMove(move);

        m_ply++; m_plyqs++; m_selPly = std::max(m_selPly, m_ply);
        D( m_debug.Increment("Quiescence: MakeMove (QPly > 0)") );

        int score = -QuiescenceSearch(board, -beta, -alpha);
        
        board.TakeMove(move);
        m_ply--; m_plyqs--;

        if(m_limits.Stopped()) return 0;

        D( BoardIdentity aft = BoardIntegrityChecker::GenerateBoardIdentity(board); );
        D( assert(bef == aft) );

        if(score > bestScore)
            bestScore = score;

        if(score >= beta) {
            m_tt.Store(board.ZKey(), bestScore, TTENTRY_TYPE::LOWER_BOUND, move, 0, m_ply, m_searchCount, standPat);
            return score;
        }

        if(score > alpha)
            alpha = score;
    }

    return bestScore;
}

// Late Move Reductions: reduce the search depth for less-promising moves.
int Search::LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV) {
    assert(moveScore  >= 0 && moveScore  <= LOG_TABLE_SIZE - 1);
    assert(depth      >= 0 && depth      <= LOG_TABLE_SIZE - 1);
    assert(moveNumber >= 0 && moveNumber <= LOG_TABLE_SIZE - 1);

    int reduction = 0;
    int lmr_value = 0;

    // Logarithmic scaling for smooth reductions
    int logScore = LogTable[moveScore + 1];
    int logDepth = LogTable[depth];
    int logMoveNumber = LogTable[moveNumber];

    // Coefficients are scaled by this amount to perform integer calculations
    const int MULT_FACTOR = 100; 

    // History moves
    if(moveScore <= Scorer::HISTORY_MAX) {
        lmr_value = -50 - 200*(isPV)
            + (
                - (20 * logScore)
                + (200 * logDepth)
                + (30 * logMoveNumber)
            ) / LOG_TABLE_SCALE
            + (
                - (30 * logScore * logDepth)
                + (15 * logScore * logMoveNumber)
            ) / (LOG_TABLE_SCALE * LOG_TABLE_SCALE);
    }

    // SEE << 0: very bad captures
    else if(moveScore >= 181 && moveScore <= 184) {
        lmr_value = 50 - 40*(isPV) + ( (135*logDepth) + (40*logMoveNumber) ) / LOG_TABLE_SCALE;
    }

    // SEE < 0: bad captures
    else if(moveScore >= 185 && moveScore <= 189) {
        lmr_value = -85 + ( (135*logDepth) + (40*logMoveNumber) ) / LOG_TABLE_SCALE;
    }

    // Killers 2,3,4: less-promising killer moves in non-PV nodes
    else if(moveScore >= 191 && moveScore <= 193 && !isPV) {
        lmr_value = -185 + ( (50*logDepth) + (165*logMoveNumber) ) / LOG_TABLE_SCALE;
    }

    reduction = lmr_value / MULT_FACTOR;
    reduction = std::clamp(reduction, 0, 4);
    return reduction;
}
