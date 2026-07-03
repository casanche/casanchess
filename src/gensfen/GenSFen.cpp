#include "gensfen/GenSFen.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <ctime>
#include <filesystem>
#include <sstream>
#include <thread>

struct CurrentPosition {
    Move bestMove = Move();
    int calculatedDepth = -1;
    int evalFail = 0;
    int evalPass = 0;
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
        assert(passed != failed);
        gameWrittenEvals += passed;
        if(passed)
            consecutiveFailedEvals = 0;
        consecutiveFailedEvals += failed;
    }
    void FinishGame() {
        totalWrittenEvals += gameWrittenEvals;
    }
};

GenSFen::GenSFen(GenSFenConfig config) : m_config(std::move(config)) {
    if(m_config.outputDir.empty())
        m_config.outputDir = "gensfen-output";
    if(m_config.bookFile.empty())
        m_config.bookFile = "../data/books/book5.epd";

    UCI_OUTPUT = false;
}

// Supported modes: 'games', 'random', 'random_benchmark' or 'benchmark'
void GenSFen::Run(const std::string& gensfen_mode, int concurrency, int depth, int maxGames) {
    std::vector<std::thread> threads;

    m_depth = depth;
    m_maxGames = maxGames > 0 ? maxGames : INFINITE;

    if(!concurrency)
        concurrency = std::thread::hardware_concurrency() - 1;
    std::cout << "Concurrency set to: " << std::to_string(concurrency) << std::endl;

    std::filesystem::create_directories(m_config.outputDir);

    if(gensfen_mode == "random_benchmark" || gensfen_mode == "benchmark") {
        RandomBenchmark(m_maxGames == INFINITE ? 100 : m_maxGames);
        return;
    }

    void (GenSFen::*function)(std::string, int);
    if(gensfen_mode == "games")
        function = &GenSFen::Games;
    else if(gensfen_mode == "random")
        function = &GenSFen::Random;
    else {
        std::cout << "GenSFen mode '" << gensfen_mode << "' not supported." << std::endl;
        return;
    }

    if(!WriteRunMetadata(gensfen_mode, concurrency, maxGames)) {
        std::cerr << "Error: Cannot write run metadata in output directory '"
                  << m_config.outputDir << "'." << std::endl;
        return;
    }

    for(int i = 1; i <= concurrency; i++) {
        std::string filename = m_config.outputDir + "/evals_generated_" + std::to_string(i) + ".epd";
        threads.push_back( std::thread(function, this, filename, i - 1) );
    }
    for(auto& th : threads) {
        th.join();
    }
}

void GenSFen::Games(std::string filename, int threadIndex) {
    std::ofstream outputFile;
    outputFile.open(filename);

    BookPositions bookPositions = ReadBook(m_config.bookFile);

    Board board;
    Search search;
    State state;
    Utils::PRNG rng(SeedForThread(threadIndex));

    const int maxGames = m_maxGames;
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
        state.NewGame();
        bool exitGame;
        do {
            CurrentPosition currentPosition;
            WriteEvals(board, search, outputFile, currentPosition, m_config.WRITE_EVALS_GAMES_SINGLE, m_config.WRITE_EVALS_GAMES_BOTH, 0, 8);

            // Make next move (random or best)
            bool doRandomMove = board.Ply() < 20 && rng.Random(0,100) < 33 && !board.IsCheck();
            Move nextMove = doRandomMove ? RandomMove(board, rng) : currentPosition.bestMove;
            board.MakeMove(nextMove);

            state.UpdateGame(currentPosition.evalPass, currentPosition.evalFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = (NoMoves(board) || nPieces <= 6 || board.Ply() > 200 || board.IsRepetitionDraw() || state.consecutiveFailedEvals >= 10);
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

void GenSFen::Random(std::string filename, int threadIndex) {
    std::ofstream outputFile;
    outputFile.open(filename);

    Board board;
    Search search;
    search.FixDepth(m_depth);
    State state;
    RandomPositionGenerator positionGenerator(SeedForThread(threadIndex));
    Search validationSearch;
    validationSearch.FixDepth(1);

    const int maxGames = m_maxGames;
    for(int n_game = 0; n_game < maxGames; n_game++) {
        std::string position;
        GenerateRandomPosition(board, position, positionGenerator, validationSearch);

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
            WriteEvals(board, search, outputFile, currentPosition, m_config.WRITE_EVALS_RANDOM_SINGLE, m_config.WRITE_EVALS_RANDOM_BOTH, 0, 0);

            board.MakeMove(currentPosition.bestMove);

            state.UpdateGame(currentPosition.evalPass, currentPosition.evalFail);

            int nPieces = PopCount(board.AllPieces());
            exitGame = (NoMoves(board) || nPieces <= 6 || board.Ply() > 40 || board.IsRepetitionDraw() || state.consecutiveFailedEvals >= 6);
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
    int64_t time_choose = 0;
    int64_t time_search = 0;
    int passed = 0;

    Board board;
    Search search;
    search.FixDepth(m_depth);
    RandomPositionGenerator positionGenerator(m_config.seed);
    Search validationSearch;
    validationSearch.FixDepth(1);

    for(int n_game = 1; n_game <= maxGames; n_game++) {
        clock.Start();
        std::string position;
        int tries = GenerateRandomPosition(board, position, positionGenerator, validationSearch);
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

int GenSFen::GenerateRandomPosition(Board& board, std::string& position,
                                    RandomPositionGenerator& positionGenerator,
                                    Search& validationSearch) {
    bool validScore = false;
    int tries = 0;

    do {
        position = positionGenerator.Generate(board);
        tries++;

        validScore = ValidateRandomPosition(board, validationSearch, 66);
    } while(!validScore);

    return tries;
}

bool GenSFen::ValidateRandomPosition(Board& board, Search& search, int scoreFilter) {
    int nPieces = PopCount(board.AllPieces());
    if(NoMoves(board) || nPieces <= 6)
        return false;

    search.FixDepth(1);
    search.IterativeDeepening(board);
    int score_active = search.BestScore();

    board.MakeNull();
    if(NoMoves(board)) {
        board.TakeNull();
        return false;
    }

    search.IterativeDeepening(board);
    int score_inactive = search.BestScore();
    board.TakeNull();

    bool evalPass = (abs(score_active) < scoreFilter) || (abs(score_inactive) < scoreFilter);
    bool evalPassBoth = abs(score_active) < 150 && abs(score_inactive) < 150;
    return evalPass && evalPassBoth;
}

// Write evals if:
// - Board not in check
// - Best move is quiet at low depths (to skip trivial captures)
// - Evaluation conditions: At least one color has [-200,200]. Both colors have [-800,800].
bool GenSFen::WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition,
                         int thresholdEval, int thresholdEvalBoth, int /*tacticalThreshold*/, uint minPly) {
    search.FixDepth(5);
    search.IterativeDeepening(board);
    currentPosition.calculatedDepth = 5;
    currentPosition.bestMove = search.BestMove();
    
    if(board.Ply() < minPly || board.IsCheck() || !search.BestMove().IsQuiet())
        return false;

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
        return false;
    }

    // Low depth. Check that move is quiet.
    search.FixDepth(5);
    search.IterativeDeepening(board);
    if(!search.BestMove().IsQuiet()) {
        board.TakeNull();
        return false;
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
        return false;
    }
    currentPosition.evalPass++;

    // Write evals to file
    std::string fenString = board.GetSimplifiedFen();
    outputFile << fenString << ";" << eval[WHITE]/100. << ";" << eval[BLACK]/100. << std::endl;

    board.TakeNull();
    return true;
}

bool GenSFen::NoMoves(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return moves.empty();
}

Move GenSFen::RandomMove(Board& board, Utils::PRNG& rng) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return MoveGenerator::RandomMove(moves, rng);
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

uint64_t GenSFen::SeedForThread(int threadIndex) const {
    return m_config.seed + static_cast<uint64_t>(threadIndex + 1);
}

bool GenSFen::WriteRunMetadata(const std::string& mode, int concurrency, int requestedMaxGames) const {
    const std::filesystem::path metadataPath = std::filesystem::path(m_config.outputDir) / "run_metadata.txt";

    std::ofstream metadata(metadataPath);
    if(!metadata.is_open())
        return false;

    metadata << "timestamp_unix=" << static_cast<long long>(std::time(nullptr)) << '\n';
    metadata << "mode=" << mode << '\n';
    metadata << "concurrency=" << concurrency << '\n';
    metadata << "depth=" << m_depth << '\n';
    metadata << "max_games_requested=" << requestedMaxGames << '\n';
    metadata << "max_games_effective=" << (m_maxGames == INFINITE ? "infinite" : std::to_string(m_maxGames)) << '\n';
    metadata << "seed_base=" << m_config.seed << '\n';
    metadata << "book_file=" << (mode == "games" ? m_config.bookFile : "-") << '\n';

    if(mode == "games" || mode == "random") {
        for(int i = 0; i < concurrency; i++) {
            metadata << "thread_" << i << ".seed=" << SeedForThread(i)
                     << " file=evals_generated_" << (i + 1) << ".epd\n";
        }
    }

    if(mode == "games") {
        metadata << "write_evals_games_single=" << m_config.WRITE_EVALS_GAMES_SINGLE << '\n';
        metadata << "write_evals_games_both=" << m_config.WRITE_EVALS_GAMES_BOTH << '\n';
    } else if(mode == "random") {
        metadata << "write_evals_random_single=" << m_config.WRITE_EVALS_RANDOM_SINGLE << '\n';
        metadata << "write_evals_random_both=" << m_config.WRITE_EVALS_RANDOM_BOTH << '\n';
    }

    return true;
}
