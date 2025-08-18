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
//    - Recapture extension in PV nodes
//
// - Reductions:
//    - Late Move Reductions: reduce search depth for late non-promising moves
//    - Razoring: reduce search depth for clearly losing positions
//
// - Heuristics:
//    - History heuristics: reward successful moves in past positions
//    - Killer moves
//    - Transposition tables
//    - Time management


#include "Search.h"

#include "Evaluation.h"
#include "MoveGenerator.h"
#include "NNUE.h" //JUST FOR THE PV
#include "Uci.h"
using namespace Sorting;

#include <algorithm> //max(), clamp()
#include <cmath> //INFINITY
#include <iomanip> //debug output
#include <format>

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

const int UCI_OUTPUT_CURRMOVE_MINTIME = 1000; //ms

// Draw contempt to discourage premature draws
#define DRAW_SCORE(ply) (ply & 1 ? 10 : -10)


// Called once when the UCI interface starts up.
Search::Search() {
    // Limits and time management
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = 3500; //ms
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    // Search state
    m_bestScore = -INFINITE_SCORE;
    m_searchCount = 0;

    ClearSearch(true);
    m_heuristics.history.Clear();

    // Clear transposition tables
    Hash::tt.Clear();
    Hash::evalCache.Clear();
    Hash::pawnHash.Clear();

    // Debug
    m_debugMode = false;
}

// Reset state for a new position search.
// Called at the beginning of each iteration in Iterative Deepening.
void Search::ClearSearch(bool fullSearchClearFlag) {
    // Node counters
    m_nodes = 0;
    m_nps = 0;

    // Time management
    m_elapsedTime = 0;
    m_stop = false;
    m_nodesTimeCheck = 0;

    // Search state
    m_bestScore = -INFINITE_SCORE;
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

    // Completely clear TT and history heuristics for a reproducible search
    if(fullSearchClearFlag) {
        Hash::tt.Clear();
        Hash::evalCache.Clear();
        Hash::pawnHash.Clear();

        m_heuristics.history.Clear();
    }
}

// Main loop: increase depth one by one and call the root search.
// Manage time, aspiration window, and UCI output.
void Search::IterativeDeepening(Board &board, bool fullSearchClearFlag) {
    m_clock.Start();

    ClearSearch(fullSearchClearFlag);
    
    D( m_debug.Increment("IterativeDeepening: _: Start") );
    m_searchCount++;

    for(m_depth = 1; m_depth <= m_maxDepth; m_depth++) {
        assert(m_ply == 0);
        assert(m_plyqs == 0);
        assert(m_nullmoveAllowed);

        m_selPly = 0;

        AspirationWindow(board, m_depth, m_bestScore);

        // Stop search if limits are reached within the loop
        if(m_stop)
            break;

        // Update counters
        m_elapsedTime = ElapsedTime();
        m_nps = static_cast<int>(1000 * m_nodes / (m_elapsedTime+1));

        // Build principal variation (PV) line from transposition table
        std::string PV;
        m_ponderMove = Move();
        Board newBoard = board;
        D(
            BoardIdentity bef = BoardIntegrityChecker::GenerateBoardIdentity(board);
            BoardIdentity aft = BoardIntegrityChecker::GenerateBoardIdentity(newBoard);
            assert(bef == aft);
        );
        for(int depth = 1; depth <= m_depth; depth++) {
            TTEntry *ttEntry = Hash::tt.Probe(newBoard.ZKey(), 0);
            if(!ttEntry) continue;
            if(ttEntry->bestMove.MoveType() == NULLMOVE) break;

            Move bestMove = ttEntry->bestMove;
            PV += bestMove.Notation();
            PV += " ";

            if(depth == 2)
                m_ponderMove = bestMove;

            newBoard.MakeMove(bestMove, false);
        }

        if(UCI_OUTPUT)
            UciOutput(PV);

        // Stop search if we used half of the allocated time, since
        // next iteration will likely use more than the allocated time
        if(m_elapsedTime > (m_allocatedTime / 2)) //check
             break;

        if(DEBUG_PRINT_STATISTICS && m_depth == m_maxDepth)
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
        int mateScore = (m_bestScore > 0) ?  MATESCORE_MAX - m_bestScore + 1
                                          : -MATESCORE_MAX - m_bestScore - 1;
        std::cout << " score mate " << mateScore / 2; //return mate in moves, not in plies
    } else {
        std::cout << " score cp " << m_bestScore;
    }
    std::cout << " time " << m_elapsedTime;
    std::cout << " nodes " << m_nodes;
    std::cout << " nps " << m_nps;
    if(m_elapsedTime > 1000)
        std::cout << " hashfull " << Hash::tt.Occupancy();
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

// Narrow alpha-beta bounds around expected score
int Search::AspirationWindow(Board& board, const int depth, const int bestScore) {
    const bool aspiration = TURNON_ASPIRATION_WINDOW && depth >= ASPIRATION_WINDOW_DEPTH && !IsMateValue(bestScore);
    if(!aspiration)
        return RootMax(board, depth, -INFINITE_SCORE, INFINITE_SCORE);

    int window = ASPIRATION_WINDOW;
    int alpha = bestScore - window;
    int beta  = bestScore + window;

    int score = RootMax(board, depth, alpha, beta);

    for(int researches = 1; (score <= alpha || score >= beta); researches++) {
        D( m_debug.Increment("AspirationWindow: Out of bounds: Researches: " + std::to_string(researches) ); );

        // Asymmetrical incremental aspiration
        window = window * ASPIRATION_WINDOW_MULTIPLIER;
        if(score <= alpha) {
            alpha = bestScore - window;
        } else if(score >= beta) {
            beta = bestScore + window;
        }

        if(researches == 4 || IsMateValue(score)) {
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

    Move bestMove;
    int score;

    int alphaOriginal = alpha; // Save the original alpha for TT logic

    MoveList moves = MoveGenerator::GenerateMoves(board);

    D( if(depth == 1) P("Number of moves in root position: " << moves.size()) );

    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

    int moveNumber = 0;
    for(auto move : moves) {
        moveNumber++;
        bool isPV = (moveNumber == 1);

        if(DEBUG_SEARCH_TREE)
            P( "RootMax: " << move.Notation() );

        if(UCI_OUTPUT) {
            //Uci output: show the root move number under analysis
            if(m_elapsedTime > UCI_OUTPUT_CURRMOVE_MINTIME) {
                std::cout << "info currmovenumber " << moveNumber;
                std::cout << " currmove " << move.Notation();
                std::cout << std::endl;
            }
        }

        board.MakeMove(move);
        m_ply++; m_nodes++;

        // -------- Principal Variation Search (PVS) -----------
        // PV move: full window
        // Other moves: zero window
        if(isPV)
            score = -NegaMax(board, depth-1, -beta, -alpha, true);
        else {
            score = -NegaMax(board, depth-1, -alpha-1, -alpha, false);

            // Score within window: new PV found! Re-search with full window
            if(score > alpha && score < beta) {
                score = -NegaMax(board, depth-1, -beta, -alpha, true);
            }
        }

        board.TakeMove(move);
        m_ply--;

        if(score > alpha) {
            D( m_debug.Increment("RootMax: AlphaBeta: Update: BestMove (score > alpha)") );
            alpha = score;
            bestMove = move;
        }

        // Not useful to store in TT due to aspiration window
        if(score >= beta) {
            D( m_debug.Increment("RootMax: AlphaBeta: Beta Cutoff (score >= beta)") );
            break;
        }

        if(m_stop || TimeOver()) {
            m_stop = true;
            break;
        }
    }

    // Store "exact" score in transposition table if search finished within search bounds
    bool outOfLimits = alpha <= alphaOriginal || alpha >= beta;
    if(!m_stop && !outOfLimits) {
        m_bestMove = bestMove;
        m_bestScore = alpha;

        D( m_debug.Increment("RootMax: AlphaBeta: Exact") );
        Hash::tt.Store(board.ZKey(), m_bestScore, TTENTRY_TYPE::EXACT, m_bestMove, depth, m_ply, m_searchCount);
    }

    return alpha;
}

// Negamax search: core recursive alpha-beta search.
// Implements most of the engine's search logic.
int Search::NegaMax(Board &board, int depth, int alpha, int beta, bool isPV) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);

    D( m_debug.Increment("NegaMax: _: Entering function") );

    // --------- Termination checks -----------
    m_nodesTimeCheck++;
    if( m_stop || NodeLimit() || TimeOver() ) {
        m_stop = true;
        return 0;
    }

    // --------- Draw detection: repetition and 50-move rule -----------
    if(board.FiftyRule() >= 100) {
        D( m_debug.Increment("NegaMax: Draw: FiftyRule") );
        return DRAW_SCORE(m_ply);
    }
    if(board.IsRepetitionDraw(m_ply)) {
        D( m_debug.Increment("NegaMax: Draw: Repetition") );
        return DRAW_SCORE(m_ply);
    }

    //---------- Mate distance pruning -------------
    // Prevents the search from reporting mates that are too far away
    // TODO: should be different for PV nodes?
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
    // TODO: verify the condition !inCheck
    if(depth <= 0 && !inCheck) {
        return QuiescenceSearch(board, alpha, beta, isPV);
    }

    D( m_debug.Increment("NegaMax: _: Hits") );

    // --------- Transposition table probe --------
    Move bestMove; // For later storage in the TT
    int bestScore = -INFINITE_SCORE;
    int alphaOriginal = alpha; // For TT entry type calculation
    
    TTEntry* ttEntry = Hash::tt.Probe(board.ZKey(), depth);
    if(ttEntry && !isPV) {
        D( m_debug.Increment("NegaMax: TT Hit") );
        int score = Hash::tt.ScoreFromHash(ttEntry->score, m_ply);
        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && score >= alpha && score <= beta) )
        {
            D( m_debug.Increment("NegaMax: TT Hit: Cut-Off") );
            return score;
        }
    }

    // Calculate static evaluation once, used for pruning heuristics
    int eval = 0;
    if(!inCheck) {
        D( m_debug.Increment("NegaMax: _: Call to Evaluation") );
        eval = Evaluation::Evaluate(board);
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

    // --------- Razoring -------------
    // Reduction in hopeless nodes (eval << alpha)
    if(depth == 3 && eval + 1150 <= alpha && !isPV && !extension && Evaluation::AreHeavyPieces(board))
    {
        D( m_debug.Increment("NegaMax: Pruning: Razoring") );
        D( m_debug.Increment("NegaMax: Pruning: Razoring - Depth " + std::to_string(depth)) );
        depth--;
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
        int nullScore = -NegaMax(board, nullDepth, -beta, -beta + 1, false);

        board.TakeNull();
        m_ply--;
        m_nullmoveAllowed = true;

        if(nullScore >= beta) {
            D( m_debug.Increment("NegaMax: Pruning: NullMove: Beta Cutoff") );
            D( m_debug.Increment("NegaMax: Pruning: NullMove: Beta Cutoff - Depth " + std::to_string(depth)) );
            if(IsMateValue(nullScore))
                nullScore = beta;  // Avoid reporting false mates in zugzwang
            Hash::tt.Store(board.ZKey(), nullScore, TTENTRY_TYPE::LOWER_BOUND, Move(), nullDepth, m_ply, m_searchCount);
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
            return DRAW_SCORE(m_ply); // Stalemate
        }
    }

    //----- One-reply extension -------
    if( moves.size() == 1 ) {
        D( m_debug.Increment("NegaMax: Extension: One-reply") );
        extension++;
    }

    // --------- Move ordering ---------
    // Order moves to maximize search efficiency (hash move, captures, killers, history...)
    SortMoves(board, moves, Hash::tt, m_heuristics, m_ply);

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
        if(!TURNOFF_FUTILITY && !isPV && !childPV && !inCheck && !IsMateValue(alpha)
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

        // ---- Pawn to 7th/8th extension -----
        // if(move.PieceType() == PAWN
        //     && RelativeRank(board.ActivePlayer(), move.ToSq()) >= RANK7
        // )
        //     extension++;

        // ----- Recapture extension ------
        // Extend if the move is a recapture of the same piece type
        if(!extension
            && move.MoveType() == CAPTURE
            && board.LastMove().MoveType() == CAPTURE
            && move.ToSq() == board.LastMove().ToSq()
            && move.CapturedType() == board.LastMove().CapturedType()
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
        m_ply++; m_nodes++;

        // -------- Principal Variation Search (PVS) -----------
        int fullDepth = depth - 1 + extension + localExtension;
        int reducedDepth = std::max(0, fullDepth - reduction);

        // PV move: full window, full depth
        // Other moves: zero window, reduced depth
        if(isPV && childPV)
            score = -NegaMax(board, fullDepth, -beta, -alpha, true);
        else {
            score = -NegaMax(board, reducedDepth, -alpha-1, -alpha, false);

            // Reduced search failed high: re-search with full depth
            if(reduction && score > alpha) {
                score = -NegaMax(board, fullDepth, -alpha-1, -alpha, false);
            }
            // Score within window: new PV found! Re-search with full window and depth
            if(score > alpha && score < beta) { // 'score < beta' needed in fail-soft schemes
                score = -NegaMax(board, fullDepth, -beta, -alpha, true);
            }
        }

        board.TakeMove(move);
        m_ply--;

        if(score > bestScore) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Update: BestMove") );
            bestScore = score;
            bestMove = move;
        }

        if(score >= beta) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Cutoff") );

            Hash::tt.Store(board.ZKey(), score, TTENTRY_TYPE::LOWER_BOUND, move, depth, m_ply, m_searchCount);

            // Update heuristics
            if( move.IsQuiet() ) {
                m_heuristics.killer.Update(move, m_ply);
                m_heuristics.history.GoodHistory(move, board.ActivePlayer(), depth);
            }

            return score;
        }

        if(score > alpha) {
            D( m_debug.Increment("NegaMax: AlphaBeta: Update: Alpha") );
            alpha = score;
        }

    } // End move loop

    // Store score in transposition table
    if(bestMove.MoveType() != 0) {
        bool alphaWithinBounds = (alpha > alphaOriginal);
        alphaWithinBounds ? D( m_debug.Increment("NegaMax: AlphaBeta: Exact") )
                          : D( m_debug.Increment("NegaMax: AlphaBeta: UpperBound") );
        TTENTRY_TYPE type = alphaWithinBounds ? TTENTRY_TYPE::EXACT
                                              : TTENTRY_TYPE::UPPER_BOUND;
        Hash::tt.Store(board.ZKey(), bestScore, type, bestMove, depth, m_ply, m_searchCount);
    }

    return bestScore;
}

// Quiescence search: extends the search at the leaf nodes to avoid the horizon effect
// in tactical sequences (captures and checks).
int Search::QuiescenceSearch(Board &board, int alpha, int beta, bool isPV) {
    assert(alpha >= -INFINITE_SCORE && beta <= INFINITE_SCORE && alpha < beta);
    assert(m_ply <= MAX_PLY);

    D( m_debug.Increment("Quiescence: _: Hits"); );
    D( if(m_plyqs <= 2 || m_plyqs % 5 == 0) m_debug.Increment("Quiescence: QPly " + std::format("{:03}", m_plyqs)) );

    // Prevent infinite recursion in rare cases (unlikely to occur)
    if (m_plyqs >= MAX_QS_PLIES) {
        D( m_debug.Increment("Quiescence: MAX_QS_PLIES reached (unlikely)") );
        return Evaluation::Evaluate(board);
    }

    int bestScore = -INFINITE_SCORE;
    bool inCheck = board.IsCheck();

    // Stand pat: static evaluation of current position
    int standPat = 0;
    if(!inCheck) {
        // Probe evaluation cache first (modifies standPat if hit)
        const bool cacheHit = Hash::evalCache.Probe(board.ZKey(), standPat);
        if(cacheHit) {
            D( m_debug.Increment("Quiescence: StandPat: EvalCache hit") );
        } else {
            D( m_debug.Increment("Quiescence: StandPat: Evaluation called") );
            standPat = Evaluation::Evaluate(board);
            Hash::evalCache.Store(board.ZKey(), standPat);
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

    // Probe transposition table.
    // Only non-PV nodes: PV nodes require the most accurate score possible.
    TTEntry* ttEntry = Hash::tt.Probe(board.ZKey(), 0);
    if(ttEntry && !isPV) {
        int score = Hash::tt.ScoreFromHash(ttEntry->score, m_ply);
        if( (ttEntry->type == TTENTRY_TYPE::UPPER_BOUND && score <= alpha)
            || (ttEntry->type == TTENTRY_TYPE::LOWER_BOUND && score >= beta)
            || (ttEntry->type == TTENTRY_TYPE::EXACT && score >= alpha && score <= beta) )
        {
            D( m_debug.Increment("Quiescence: TT Hit") );
            return score;
        }
    }

    // Generate captures.
    // If in check, generate check evasions.
    D( m_debug.Increment("Quiescence: GenerateMoves") );
    MoveList moves = inCheck ? MoveGenerator::GenerateEvasionMoves(board)
                             : MoveGenerator::GenerateTacticalMoves(board);

    // Checkmate or stalemate
    if( moves.empty() ) {
        if(inCheck) {
            D( m_debug.Increment("Quiescence: EmptyMoves: Checkmate") );
            return -MATESCORE_MAX + m_ply; // Checkmate
        }
        // Generate all the moves to verify stalemate
        else if( MoveGenerator::GenerateMoves(board).size() == 0 ) {
            D( m_debug.Increment("Quiescence: EmptyMoves: Stalemate") );
            return DRAW_SCORE(m_ply); // Stalemate
        }
    }

    // Different move ordering for efficiency
    inCheck ? SortEvasions(board, moves)
            : SortTactical(board, moves);

    for(auto move : moves) {

        if(!inCheck && move.MoveType() == CAPTURE) {
            const int seeValue = SEE::FromScore(move.Score());

            // --- Static SEE Pruning ---
            // Quick filter to discard tactically hopeless captures
            const int SEE_PRUNING_THRESHOLD = -250;
            if(seeValue < SEE_PRUNING_THRESHOLD) {
                D( m_debug.Increment("Quiescence: Pruning: SEE << 0") );
                continue;
            }
            
            // --- Delta Pruning ---
            // Calculate a heuristic score for futility pruning. Depending on the SEE:
            //  - For safe captures (SEE >= 0): an optimistic score using the maximum potential gain to represent the move impact.
            //  - For sacrifices    (SEE  < 0): a realistic estimate using the material cost to determine if the sacrifice is affordable.
            int estimatedScore;
            if(seeValue >= 0) {
                const int DELTA_MARGIN_SAFE = 90;
                estimatedScore = standPat + DELTA_MARGIN_SAFE + SEE::MATERIAL_VALUES[move.CapturedType()];
            } else {
                const int DELTA_MARGIN_RISKY = 0;
                estimatedScore = standPat + DELTA_MARGIN_RISKY + seeValue;
            }

            if(estimatedScore < alpha) {
                if(estimatedScore > bestScore)
                    bestScore = estimatedScore;
                continue;
            }
        }
        
        D( BoardIdentity bef = BoardIntegrityChecker::GenerateBoardIdentity(board); );

        board.MakeMove(move);

        m_ply++; m_plyqs++; m_nodes++; m_selPly = std::max(m_selPly, m_ply);
        D( m_debug.Increment("Quiescence: MakeMove (QPly > 0)") );

        int score = -QuiescenceSearch(board, -beta, -alpha, false);
        
        board.TakeMove(move);
        m_ply--; m_plyqs--;

        D( BoardIdentity aft = BoardIntegrityChecker::GenerateBoardIdentity(board); );
        D( assert(bef == aft) );

        if(score > bestScore)
            bestScore = score;

        if(score >= beta)
            return score;

        if(score > alpha)
            alpha = score;
    }

    return bestScore;
}

// Check if search should be stopped due to time limits.
// Calculate every N nodes to avoid expensive time checks.
bool Search::TimeOver() {
    if(m_nodesTimeCheck > 5000) {
        m_nodesTimeCheck = 0;
        m_elapsedTime = ElapsedTime();
        if(m_elapsedTime > m_allocatedTime || m_elapsedTime > m_forcedTime)
            return true;
    }
    return false;
}

// Set search limits from the UCI interface.
// For normal timed games, estimate the time to spend in the search (allocatedTime).
// Modes:
// - infinite: search until manually stopped
// - depth: search for a fixed depth
// - moveTime: search for a fixed time
// - nodes: search for a fixed number of nodes
void Search::AllocateLimits(Board &board, Limits limits) {
    m_limits = limits;
    m_nodes = 0;
    m_clock.Start();

    // Defaults
    m_maxDepth = MAX_DEPTH;
    m_allocatedTime = INFINITE;
    m_forcedTime = INFINITE;
    m_forcedNodes = INFINITE_U64;

    // Special search modes
    if(limits.infinite) { Infinite(); return; }
    if(limits.depth)    { FixDepth(limits.depth); return; }
    if(limits.moveTime) { FixTime(limits.moveTime); return; }
    if(limits.nodes)    { FixNodes(limits.nodes); return; }
    
    // Time estimation in normal games
    limits.movesToGo = 20 + 20 * limits.ponderhit;

    COLOR color = board.ActivePlayer();

    int myTime   = (color == WHITE) ? limits.wtime : limits.btime;
    int yourTime = (color == WHITE) ? limits.btime : limits.wtime;

    int myInc    = (color == WHITE) ? limits.winc  : limits.binc;

    m_allocatedTime = myTime / limits.movesToGo + myInc;

    if(m_debugMode)
        std::cout << "info string TimeAllocation " <<  myTime << " " << yourTime << " " << m_allocatedTime << std::endl;
}

// Late Move Reductions: reduce the search depth for less-promising moves.
int Search::LateMoveReductions(int moveScore, int depth, int moveNumber, bool isPV) {
    assert(moveScore  >= 0 && moveScore + 1  <= LOG_TABLE_SIZE - 1);
    assert(depth      >= 0 && depth          <= LOG_TABLE_SIZE - 1);
    assert(moveNumber >= 0 && moveNumber     <= LOG_TABLE_SIZE - 1);

    if(moveScore >= 195)
        return 0;

    // Logarithmic scaling for smooth reductions
    const int logScore = LogTable[moveScore + 1];
    const int logDepth = LogTable[depth];
    const int logMoveNumber = LogTable[moveNumber];

    // Coefficients are scaled by this amount to perform integer calculations
    constexpr int MULT_FACTOR = 100;

    // Coefficients
    constexpr int LMR_COMMON = 50;
    constexpr int LMR_ISPV = -100;
    constexpr int LMR_LOGTERM_DEPTH = 160;
    constexpr int LMR_LOGTERM_MOVENUMBER = 30;
    constexpr int LMR_LOGTERM_HISTORY_SCORE = -40;
    constexpr int LMR_BADCAPTURES = 20;
    constexpr int LMR_BADCAPTURE_TIER = -35;
    constexpr int LMR_KILLERS = -120;
    constexpr int LMR_KILLER_TIER = -50;

    // Common terms
    int directTerms = LMR_COMMON + LMR_ISPV * isPV;
    int logTerms = LMR_LOGTERM_DEPTH * logDepth + LMR_LOGTERM_MOVENUMBER * logMoveNumber;

    // History moves
    if(moveScore < 180) {
        logTerms += LMR_LOGTERM_HISTORY_SCORE * logScore;
    }
    // Bad captures
    else if(moveScore >= 181 && moveScore <= 189) {
        const int badCaptureTier = moveScore - 181;
        directTerms += LMR_BADCAPTURES + LMR_BADCAPTURE_TIER * badCaptureTier; // Probar -35 --> -30 (ya que direct 20 y 60 son equivalentes) {20,-30}, {60,-40}
    }
    // Killers
    else if(moveScore >= 191 && moveScore <= 194) {
        const int killerTier = moveScore - 191;
        directTerms += LMR_KILLERS + LMR_KILLER_TIER * killerTier; // Probar amarillo: -120 --> -110, añadiendo quizá -60* / {-150,-40}, {-100,-60}
    }

    int reduction = directTerms + (logTerms / LOG_TABLE_SCALE); // Log table was scaled by this amount for integer computation
    reduction /= MULT_FACTOR;
    
    reduction = std::clamp(reduction, 0, 3 + depth / 5);
    return reduction;
}
