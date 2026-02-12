#include "gensfen/GenSFen.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "Evaluation.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <filesystem>
#include <sstream>
#include <thread>
#include <utility>

const int TACTICAL_THRESHOLD = 120;
const int MAX_NPIECES = 10;

struct CurrentPosition {
    Move bestMove = Move();
    int calculatedDepth = -1;
    int scoreFail = 0;
    int scorePass = 0;
};

struct State {
    int consecutiveFailedEvals = 0;
    int gameWrittenEvals = 0;
    int totalWrittenEvals = 0;

    void NewGame() {
        gameWrittenEvals = 0;
        consecutiveFailedEvals = 0;
    }
    void UpdateGame(bool passed, bool failed) {
        assert(!(passed && failed));
        gameWrittenEvals += passed;
        if(passed) {
            consecutiveFailedEvals = 0;
        } else if(failed) {
            consecutiveFailedEvals++;
        }
    }
    void FinishGame() {
        totalWrittenEvals += gameWrittenEvals;
    }
};

GenSFen::GenSFen(GenSFenConfig config) : m_config(std::move(config)) {
    UCI_OUTPUT = false;
}

// Supported modes: 'games', 'random', 'benchmark'
void GenSFen::Run(const std::string& gensfen_mode, int concurrency, int depth) {
    m_depth = depth;
    std::vector<std::thread> threads;

    if(!concurrency)
        concurrency = std::thread::hardware_concurrency() - 1;
    std::cout << "Concurrency set to: " << std::to_string(concurrency) << std::endl;

    if(gensfen_mode == "benchmark") {
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

    std::error_code ec;
    std::filesystem::create_directories(m_config.outputDir, ec);
    if(ec) {
        std::cerr << "Error: Cannot create output directory '" << m_config.outputDir
                  << "': " << ec.message() << std::endl;
        return;
    }

    for(int i = 1; i <= concurrency; i++) {
        std::string filename =
            (std::filesystem::path(m_config.outputDir) /
             ("evals_generated_" + gensfen_mode + "_" + std::to_string(i) + ".epd"))
                .string();
        threads.push_back( std::thread(function, this, filename) );
    }
    for(auto& th : threads) {
        th.join();
    }
}

void GenSFen::Games(std::string filename) {
    constexpr int maxGames = INFINITE;
    constexpr int SCORE_THRESHOLD_SINGLE = 125;
    constexpr int SCORE_THRESHOLD_BOTH = 4 * SCORE_THRESHOLD_SINGLE;

    std::ofstream outputFile;
    outputFile.open(filename);
    if(!outputFile.is_open()) {
        std::cerr << "Error: Cannot open output file: " << filename << std::endl;
        return;
    }

    if(m_config.bookFile.empty()) {
        std::cerr << "Error: Book file not configured." << std::endl;
        return;
    }
    BookPositions bookPositions = ReadBook(m_config.bookFile);
    if(bookPositions.empty()) {
        std::cerr << "Error: Book file is empty or unreadable: " << m_config.bookFile << std::endl;
        return;
    }

    Board board;
    Search search;
    State state;
    Utils::PRNG rng;

    for(int n_game = 0; n_game < maxGames; n_game++) {
        //New starting position
        uint32_t randomIndex = rng.Random(0, static_cast<uint32_t>(bookPositions.size())-1);
        std::string position = bookPositions[randomIndex];
        board.SetFen(position);

        //To avoid overlap in standard output due to multiple threads
        std::stringstream ss;
        ss << "thread: " << std::this_thread::get_id()
           << " game " << std::to_string(n_game)
           << ", " << position << ", ";

        //Game Loop
        bool exitGame;
        state.NewGame();
        do {
            CurrentPosition currentPosition;
            WriteEvals(board, search, outputFile, currentPosition,
                SCORE_THRESHOLD_SINGLE,
                SCORE_THRESHOLD_BOTH,
                TACTICAL_THRESHOLD, 4);

            Move nextMove = DoRandomMove(board, rng) ? RandomMove(board) : currentPosition.bestMove;
            board.MakeMove(nextMove);

            state.UpdateGame(currentPosition.scorePass, currentPosition.scoreFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = (NoMoves(board)
                || nPieces <= MAX_NPIECES
                || board.Ply() > 160
                || board.IsRepetitionDraw()
                || board.FiftyRule() > 60
                || state.consecutiveFailedEvals >= 10);
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
    constexpr int SCORE_THRESHOLD_SINGLE = 75;
    constexpr int SCORE_THRESHOLD_BOTH = 4 * SCORE_THRESHOLD_SINGLE;

    std::ofstream outputFile;
    outputFile.open(filename);
    if(!outputFile.is_open()) {
        std::cerr << "Error: Cannot open output file: " << filename << std::endl;
        return;
    }

    Board board;
    Search search;
    search.FixDepth(m_depth);
    State state;
    RandomPositionGenerator positionGenerator;

    const int maxGames = INFINITE;
    for(int n_game = 0; n_game < maxGames; n_game++) {
        std::string position;
        GenerateRandomPosition(board, position, positionGenerator);

        //To avoid overlap in standard output due to multiple threads
        std::stringstream ss;
        ss << "thread: " << std::this_thread::get_id()
           << " random position " << std::to_string(n_game)
           << ", " << position << ", ";

        //Game loop
        bool exitGame;
        state.NewGame();
        do {
            CurrentPosition currentPosition;
            WriteEvals(board, search, outputFile, currentPosition,
                SCORE_THRESHOLD_SINGLE,
                SCORE_THRESHOLD_BOTH,
                TACTICAL_THRESHOLD, 0);

            board.MakeMove(currentPosition.bestMove);
            state.UpdateGame(currentPosition.scorePass, currentPosition.scoreFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = NoMoves(board)
                || nPieces <= MAX_NPIECES
                || board.Ply() > 40
                || board.IsRepetitionDraw()
                || state.consecutiveFailedEvals >= 6
                || state.gameWrittenEvals >= 6;
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
    const int SCORE_PASS_SINGLE = 150;
    const int SCORE_PASS_BOTH = 500;

    Utils::Clock clock, global_clock;
    global_clock.Start();
    int64_t time_choose = 0;
    int64_t time_search = 0;
    u64 sum_time_choose = 0;
    u64 sum_time_search = 0;
    int passed = 0;

    Board board;
    Search search;
    search.FixDepth(m_depth);
    RandomPositionGenerator positionGenerator;

    for(int n_game = 1; n_game <= maxGames; n_game++) {
        clock.Start();
        std::string position;
        int tries = GenerateRandomPosition(board, position, positionGenerator);
        time_choose = clock.Elapsed();
        sum_time_choose += time_choose;

        clock.Start();

        search.IterativeDeepening(board, false);
        int score_active = search.BestScore();

        board.MakeNull();
        search.IterativeDeepening(board, false);
        int score_inactive = search.BestScore();
        board.TakeNull();

        time_search = clock.Elapsed();
        sum_time_search += time_search;

        bool evalPass = (abs(score_active) < SCORE_PASS_SINGLE) || (abs(score_inactive) < SCORE_PASS_SINGLE);
        bool evalPassBoth = abs(score_active) < SCORE_PASS_BOTH && abs(score_inactive) < SCORE_PASS_BOTH;
        if(evalPass && evalPassBoth) {
            passed++;
            P("FEN: " << position);
        }

        P("game " << n_game << " tries " << tries
                  << " scoreActive " << score_active << " scoreInactive " << score_inactive
                  << " time_choose " << time_choose
                  << " time_search " << time_search);

    } //games

    P("passed: " << passed
        << " total time " << global_clock.Elapsed()
        << " choose time " << sum_time_choose
        << " search time " << sum_time_search);
}

bool GenSFen::ValidateRandomPosition(Board& board, Search& search, int scoreFilter) {
    // Quick filters
    int nPieces = PopCount(board.AllPieces());
    if(NoMoves(board) || board.IsCheck() || nPieces <= MAX_NPIECES)
        return false;

    // Active side: score and tactical check
    search.IterativeDeepening(board, false);
    int activeScore = search.BestScore();
    if(abs(activeScore) > scoreFilter)
        return false;
    
    int activeEval = Evaluation::Evaluate(board);
    if(abs(activeScore - activeEval) > TACTICAL_THRESHOLD)
        return false;

    // Inactive side: validate after null move
    board.MakeNull();
    
    bool valid = true;
    if(NoMoves(board) || board.IsCheck()) {
        valid = false;
    } else {
        search.IterativeDeepening(board, false);
        int inactiveScore = search.BestScore();
        if(abs(inactiveScore) > scoreFilter) {
            valid = false;
        } else {
            int inactiveEval = Evaluation::Evaluate(board);
            if(abs(inactiveScore - inactiveEval) > TACTICAL_THRESHOLD)
                valid = false;
        }
    }
    
    board.TakeNull();  // Always restore board state
    return valid;
}

int GenSFen::GenerateRandomPosition(Board& board, std::string& position, RandomPositionGenerator& positionGenerator) {
    const int SCORE_FILTER = 250;
    const int VALIDATION_DEPTH = 4;

    Search validationSearch;
    validationSearch.FixDepth(VALIDATION_DEPTH);

    int tries = 0;
    while(true) {
        tries++;
        position = positionGenerator.Generate(board);
        
        if(ValidateRandomPosition(board, validationSearch, SCORE_FILTER))
            break;
    }

    return tries;
}

// Write evals if:
// - Board not in check
// - Best move is quiet at low depths (to skip trivial captures)
// - Evaluation conditions: At least one color has [-200,200]. Both colors have [-800,800].
bool GenSFen::WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition, int thresholdEval, int thresholdEvalBoth, int tacticalThreshold, uint minPly) {  
    search.FixDepth(m_depth-2);
    search.IterativeDeepening(board);
    currentPosition.calculatedDepth = m_depth-2;
    currentPosition.bestMove = search.BestMove();
    int eval = Evaluation::Evaluate(board);
    
    if(NoMoves(board) || board.IsCheck() || board.Ply() < minPly
        ||(search.BestScore() - eval) > tacticalThreshold
        || IsWinValue(search.BestScore())
    )
        return false;

    search.FixDepth(m_depth);
    search.IterativeDeepening(board);
    currentPosition.calculatedDepth = m_depth;
    currentPosition.bestMove = search.BestMove();

    int score[2]; //COLOR
    COLOR color = board.ActivePlayer();
    score[color] = search.BestScore();

    // Enemy move
    board.MakeNull();

    // Check that enemy move is possible
    if(NoMoves(board) || board.IsCheck()) {
        board.TakeNull();
        return false;
    }

    // Low depth. Check that move is quiet.
    search.FixDepth(m_depth-2);
    search.IterativeDeepening(board);
    eval = Evaluation::Evaluate(board);
    if( (search.BestScore() - eval) > tacticalThreshold
        || IsWinValue(search.BestScore())
    ) {
        board.TakeNull();
        return false;
    }

    // Normal depth
    search.FixDepth(m_depth);
    search.IterativeDeepening(board);
    score[1-color] = search.BestScore();

    // Eval conditions
    int scorePass = abs(score[WHITE]) < thresholdEval || abs(score[BLACK]) < thresholdEval;
    int scorePassSoft = abs(score[WHITE]) < thresholdEvalBoth && abs(score[BLACK]) < thresholdEvalBoth;
    if(!scorePass || !scorePassSoft) {
        currentPosition.scoreFail++;
        board.TakeNull();
        return false;
    }
    currentPosition.scorePass++;

    // Write evals to file
    std::string fenString = board.GetSimplifiedFen();
    outputFile << fenString << ";" << score[WHITE]/100. << ";" << score[BLACK]/100. << std::endl;

    board.TakeNull();
    return true;
}

// Choose if the next move will be 'random' or ' best'
bool GenSFen::DoRandomMove(Board& board, Utils::PRNG& rng) {
    if(board.Ply() > 22 || board.IsCheck())
        return false;

    // Ply [minPly, 11]: 33% random
    // Ply [12, 22]: 20% random
    uint perc = board.Ply() <= 11 ? 33 : 20;
    return (rng.Random(0,100) < perc);
}

bool GenSFen::NoMoves(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return moves.empty();
}

Move GenSFen::RandomMove(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return MoveGenerator::RandomMove(moves);
}

BookPositions GenSFen::ReadBook(const std::string& bookPath) {
    std::ifstream bookFile;
    bookFile.open(bookPath);
    if(!bookFile.is_open())
        std::cerr << "Error: Cannot open epd book file: " << bookPath << std::endl;

    BookPositions bookPositions;
    std::string position;
    while( std::getline(bookFile, position) ) {
        bookPositions.push_back(position);
    }
    bookFile.close();

    return bookPositions;
}
