#include "GenSFen.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <sstream>
#include <thread>

const int CONCURRENCY = 11;

struct State {
    int consecutiveFailedEvals = 0;
    int gameWrittenEvals = 0;
    int totalWrittenEvals = 0;

    void NewGame() {
        gameWrittenEvals = 0;
        consecutiveFailedEvals = 0;
    }
    void UpdateGame(bool passed, bool failed) {
        gameWrittenEvals += passed;
        if(passed)
            consecutiveFailedEvals = 0;
        consecutiveFailedEvals += failed;
    }
    void FinishGame() {
        totalWrittenEvals += gameWrittenEvals;
    }
};

GenSFen::GenSFen() : m_rng(0) {
    UCI_OUTPUT = false;
    m_createDebugFile = false;
}

// Supported modes: 'games', 'random', 'random_benchmark' or 'benchmark'
void GenSFen::Run(const std::string& gensfen_mode) {
    std::vector<std::thread> threads;

    if(gensfen_mode == "random_benchmark" || gensfen_mode == "benchmark") {
        RandomBenchmark(100);
        return;
    }

    void (GenSFen::*function)(std::string);
    if(gensfen_mode == "games")
        function = &GenSFen::Games;
    else if(gensfen_mode == "random")
        function = &GenSFen::Random;
    else {
        std::cout << "GenSFen mode '" << gensfen_mode << "' not supported." << std::endl;
        return;
    }

    for(int i = 1; i <= CONCURRENCY; i++) {
        std::string filename = "../dev/nnue/sfen/latest/evals_latest_" + std::to_string(i) + ".epd";
        threads.push_back( std::thread(function, this, filename) );
    }
    for(auto& th : threads) {
        th.join();
    }
}

void GenSFen::Games(std::string filename) {
    std::ofstream outputFile;
    outputFile.open(filename);

    BookPositions bookPositions = ReadBook("../data/books/book5.epd");

    Board board;
    Search search;
    State state;

    const int maxGames = INFINITE;
    for(int n_game = 0; n_game < maxGames; n_game++) {
        //New starting position
        std::string position = bookPositions[ m_rng.Random(0, bookPositions.size()-1) ];
        board.SetFen(position);

        //To avoid overlap in standard output due to multiple threads
        std::stringstream ss;
        ss << "thread: " << std::this_thread::get_id()
           << " game " << std::to_string(n_game)
           << ", " << position << ", ";

        //Game Loop
        state.NewGame();
        bool exitGame;
        do {
            CurrentPosition currentPosition;
            WriteEvals(board, search, outputFile, currentPosition, 200, 800, 8);

            // Make next move (random or best)
            bool doRandomMove = board.Ply() < 20 && m_rng.Random(0,100) < 33 && !board.IsCheck();
            Move nextMove = doRandomMove ? RandomMove(board) : currentPosition.bestMove;
            board.MakeMove(nextMove);

            state.UpdateGame(currentPosition.evalPass, currentPosition.evalFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = (NoMoves(board) || nPieces <= 6 || board.Ply() > 100 || board.IsRepetitionDraw() || state.consecutiveFailedEvals >= 6);
        } while(!exitGame);

        state.FinishGame();
        ss << "writtenEvals: " << state.gameWrittenEvals
           << ", consecutive fails: " << state.consecutiveFailedEvals
           << ", totalWrittenEvals: " << state.totalWrittenEvals
           << std::endl;
        std::cout << ss.str();

    } //games

    outputFile.close();
}

void GenSFen::Random(std::string filename) {
    std::ofstream outputFile;
    outputFile.open(filename);

    Board board;
    Search search;
    search.FixDepth(7);
    State state;

    const int maxGames = INFINITE;
    for(int n_game = 0; n_game < maxGames; n_game++) {
        std::string position;
        GenerateRandomPosition(board, position);

        //To avoid overlap in standard output due to multiple threads
        std::stringstream ss;
        ss << "thread: " << std::this_thread::get_id()
           << " random position " << std::to_string(n_game)
           << ", " << position << ", ";

        //Game loop
        state.NewGame();
        bool exitGame;
        do {
            CurrentPosition currentPosition;
            WriteEvals(board, search, outputFile, currentPosition, 100, 250, 0);

            board.MakeMove(currentPosition.bestMove);

            state.UpdateGame(currentPosition.evalPass, currentPosition.evalFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = (NoMoves(board) || nPieces <= 6 || board.Ply() > 100 || board.IsRepetitionDraw() || state.consecutiveFailedEvals >= 6);
        } while(!exitGame);

        state.FinishGame();
        ss << "writtenEvals: " << state.gameWrittenEvals
           << ", consecutive fails: " << state.consecutiveFailedEvals
           << ", totalWrittenEvals: " << state.totalWrittenEvals
           << std::endl;
        std::cout << ss.str();

    } //games

    outputFile.close();
}

void GenSFen::RandomBenchmark(int maxGames) {
    Utils::Clock clock, global_clock;
    global_clock.Start();
    int time_choose = 0;
    int time_search = 0;
    int passed = 0;

    Board board;
    Search search;
    search.FixDepth(7);

    for(int n_game = 1; n_game <= maxGames; n_game++) {
        clock.Start();
        std::string position;
        int tries = GenerateRandomPosition(board, position);
        time_choose = clock.Elapsed();

        clock.Start();

        search.IterativeDeepening(board);
        int score_active = search.BestScore();

        board.MakeNull();
        search.IterativeDeepening(board);
        int score_inactive = search.BestScore();
        board.TakeNull();

        time_search = clock.Elapsed();

        bool evalPass = (abs(score_active) < 100) || (abs(score_inactive) < 100);
        bool evalPassBoth = abs(score_active) < 250 && abs(score_inactive) < 250;
        if(evalPass && evalPassBoth) {
            passed++;
        }

        P("game " << n_game << " tries " << tries
                  << " time_choose " << time_choose
                  << " time_search " << time_search);

    } //games

    P("passed: " << passed << " total time " << global_clock.Elapsed() );
}

// Write evals if:
// - Board not in check
// - Best move is quiet at low depths (to skip trivial captures)
// - Evaluation conditions: At least one color has [-200,200]. Both colors have [-800,800].
void GenSFen::WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition, uint thresholdEval, uint thresholdEvalBoth, uint minPly) {
    search.FixDepth(5);
    search.IterativeDeepening(board);
    currentPosition.calculatedDepth = 5;
    currentPosition.bestMove = search.BestMove();
    
    if(board.Ply() < minPly || board.IsCheck() || !search.BestMove().IsQuiet())
        return;

    search.FixDepth(7);
    search.IterativeDeepening(board);
    currentPosition.calculatedDepth = 7;
    currentPosition.bestMove = search.BestMove();

    int eval[2]; //COLOR
    COLOR color = board.ActivePlayer();
    eval[color] = search.BestScore();

    // Enemy move
    board.MakeNull();

    // Check that enemy move is possible
    if(NoMoves(board) || board.IsCheck()) {
        board.TakeNull();
        return;
    }

    // Low depth. Check that move is quiet.
    search.FixDepth(5);
    search.IterativeDeepening(board);
    if(!search.BestMove().IsQuiet()) {
        board.TakeNull();
        return;
    }

    // Normal depth
    search.FixDepth(7);
    search.IterativeDeepening(board);
    eval[1-color] = search.BestScore();

    // Eval conditions
    int evalPass = abs(eval[WHITE]) < thresholdEval || abs(eval[BLACK]) < thresholdEval;
    int evalPassSoft = abs(eval[WHITE]) < thresholdEvalBoth && abs(eval[BLACK]) < thresholdEvalBoth;
    if(!evalPass || !evalPassSoft) {
        currentPosition.evalFail++;
        board.TakeNull();
        return;
    }
    currentPosition.evalPass++;

    // Write evals to file
    std::string fenString = board.GetSimplifiedFen();
    outputFile << fenString << ";" << eval[WHITE]/100. << ";" << eval[BLACK]/100. << std::endl;

    board.TakeNull();
}

int GenSFen::GenerateRandomPosition(Board& board, std::string& position) {
    Search search_depth1;
    search_depth1.FixDepth(1);

    bool validScore = false;
    int tries = 0;

    do {
        position = board.SetFenRandom();
        tries++;

        int nPieces = PopCount(board.AllPieces());
        if(NoMoves(board) || nPieces <= 6)
            continue;
        search_depth1.IterativeDeepening(board);
        int score_active = search_depth1.BestScore();

        board.MakeNull();
        if(NoMoves(board))
            continue;
        search_depth1.IterativeDeepening(board);
        int score_inactive = search_depth1.BestScore();

        bool evalPass = (abs(score_active) < 66) || (abs(score_inactive) < 66);
        bool evalPassBoth = abs(score_active) < 150 && abs(score_inactive) < 150;
        validScore = evalPass && evalPassBoth;
        if(validScore)
            board.TakeNull();
    } while(!validScore);

    return tries;
}

bool GenSFen::NoMoves(Board& board) {
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(board);
    return (moves.size() == 0);
}

Move GenSFen::RandomMove(Board& board) {
    MoveGenerator gen;
    gen.GenerateMoves(board);
    return gen.RandomMove();
}

BookPositions GenSFen::ReadBook(const std::string& bookPath) {
    std::ifstream bookFile;
    bookFile.open(bookPath);

    BookPositions bookPositions;
    std::string position;
    while( std::getline(bookFile, position) ) {
        bookPositions.push_back(position);
    }
    bookFile.close();

    return bookPositions;
}