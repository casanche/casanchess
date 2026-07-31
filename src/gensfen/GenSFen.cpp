#include "gensfen/GenSFen.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <ctime>
#include <filesystem>
#include <syncstream>
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

    // Set seed to random if not provided
    if(m_config.seed == 0) {
        Utils::PRNG_64 rng(0);
        m_config.seed = rng.Random();
    }

    if(!concurrency)
        concurrency = std::thread::hardware_concurrency() - 1;
    std::cout << "Concurrency set to: " << std::to_string(concurrency) << std::endl;

    // Output directory
    std::filesystem::path timestampPath(m_config.outputDir);
    timestampPath /= std::format("{}/{}_{:%Y%m%d}", gensfen_mode, gensfen_mode, Utils::DateTime::Now());
    std::filesystem::create_directories(timestampPath);
    m_config.outputDir = timestampPath.string();

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

    if(!WriteRunMetadata(gensfen_mode, concurrency)) {
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
    std::ofstream outputFile(filename);
    if(!outputFile.is_open()) {
        std::cerr << "Error: Cannot open output file '" << filename << "'." << std::endl;
        return;
    }

    BookPositions bookPositions = ReadBook(m_config.bookFile);
    if(bookPositions.empty()) {
        std::cerr << "Error: Book file '" << m_config.bookFile << "' is empty or cannot be read." << std::endl;
        return;
    }

    Board board;
    Search search;
    Utils::PRNG rng(SeedForThread(threadIndex));

    while(m_gamesPlayed.fetch_add(1) < m_maxGames) {
        int n_game = m_gamesPlayed.load();

        //New starting position
        uint32_t randomIndex = rng.Random(0, static_cast<uint32_t>(bookPositions.size()) - 1);
        std::string startingFen = bookPositions[randomIndex];
        board.SetFen(startingFen);

        const uint initialPly = board.Ply();

        std::vector<SavedPosition> savedPositions;
        savedPositions.reserve(512);

        int gameResult = 0;
        int adjWinCounter = 0;
        int adjDrawCounter = 0;

        bool exitGame = false;
        while(!exitGame) {
            COLOR color = board.ActivePlayer();
            int playedPlies = board.Ply() - initialPly;
            bool softRandomize = playedPlies < m_config.SOFT_RANDOMIZE_PLIES;

            SearchIteration(board, search);

            Move bestMove = search.BestMove();
            int score = search.BestScore();

            Move nextMove = bestMove;

            // Soft-randomize in the first plies.
            // Meaning: randomly choose between the top 3 moves
            if(softRandomize) {
                MoveList moves = GenSFen::SortGoodMoves(board, search);
                if(moves.size() >= 3)
                    nextMove = moves[rng.Random(0, 2)];
            } else {
                // Save the evaluation if meets the criteria
                SaveEvals(board, search, savedPositions);
            }

            // P("fen: " << board.GetFen() << ", playedPlies: " << playedPlies << ", nextMove: " << nextMove.Notation() << ", score: " << score << ", bestMove: " << bestMove.Notation() << ", softRandomize: " << softRandomize);

            // Hard-randomize in the first plies.
            // Meaning: randomly choose between all the moves
            // ...
            // bool doRandomMove = board.Ply() < 20 && rng.Random(0,100) < 33 && !board.IsCheck();
            // Move nextMove = doRandomMove ? RandomMove(board, rng) : currentPosition.bestMove;

            board.MakeMove(nextMove);

            // Checkmate or stalemate
            if(NoMoves(board)) {
                if(board.IsCheck()) {
                    gameResult = board.ActivePlayer() == WHITE ? -1 : 1;
                } else {
                    gameResult = 0;
                }
                exitGame = true;
                break;
            }

            // Draw
            if(board.FiftyRule() >= 100 || board.IsRepetitionDraw()) {
                gameResult = 0;
                exitGame = true;
                break;
            }

            // Adjudication Wins
            int scoreWhitePOV = color == WHITE ? score : -score;
            if(std::abs(score) >= m_config.ADJUDICATION_THRESHOLD_WIN) {
                adjWinCounter++;
                if(adjWinCounter >= m_config.ADJUDICATION_PLIES_WIN) {
                    gameResult = scoreWhitePOV > 0 ? 1 : -1;
                    exitGame = true;
                    break;
                }
            } else {
                adjWinCounter = 0;
            }

            // Adjudication Draws
            if(std::abs(score) <= m_config.ADJUDICATION_THRESHOLD_DRAW) {
                adjDrawCounter++;
                if(adjDrawCounter >= m_config.ADJUDICATION_PLIES_DRAW) {
                    gameResult = 0;
                    exitGame = true;
                    break;
                }
            } else {
                adjDrawCounter = 0;
            }

            // Stuck
            if(board.Ply() > m_config.MAX_PLIES) {
                gameResult = 0;
                exitGame = true;
                break;
            }
        } // Game Finished

        // Collect game global metrics
        int gameLength = board.Ply();
        int pawnCount = PopCount(board.Piece(WHITE, PAWN) | board.Piece(BLACK, PAWN));
        int heavyPiecesCount = PopCount(board.AllPieces()) - pawnCount - 2; // Exclude kings

        // Write to temporary buffer
        std::stringstream ss;
        for(const auto& savedPosition : savedPositions) {
            ss << savedPosition.fen << ";"
               << "bm " << savedPosition.bestMove << ";"
               << "ev " << savedPosition.eval << ";"
               << "r " << gameResult << ";"
               << "l " << gameLength << ";"
               << "pC " << pawnCount << ";"
               << "PC " << heavyPiecesCount
               << "\n";
        }

        // Write to file. Each thread write to its own file
        outputFile << ss.view();

        // Logs to standard output (multi-thread safe)
        std::osyncstream syncedLog(std::cout);
        syncedLog << "thread: " << std::this_thread::get_id()
                  << " game " << n_game
                  << " starting fen: " << startingFen
                  << " saved positions: " << savedPositions.size()
                  << " result " << gameResult
                  << " length " << gameLength
                  << " pC " << pawnCount
                  << " PC " << heavyPiecesCount
                  << " adjWinCounter " << adjWinCounter
                  << " adjDrawCounter " << adjDrawCounter
                  << "\n";
    }
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

        std::vector<SavedPosition> savedPositions;

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
            SaveEvals(board, search, savedPositions);

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

// Save evals if:
// - Board not in check
// - Best move is quiet at low depths (to skip captures and highly tactical positions)
bool GenSFen::SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions) {
    if(board.IsCheck()
        || !search.BestMove().IsQuiet()
        || IsWinValue(search.BestScore())
    )
        return false;

    int score = search.BestScore();
    int scoreWhitePOV = board.ActivePlayer() == WHITE ? score : -score;

    // Save evals to struct
    SavedPosition savedPosition;
    savedPosition.fen = board.GetFen();
    savedPosition.bestMove = search.BestMove().Notation();
    savedPosition.eval = scoreWhitePOV;
    savedPositions.push_back(savedPosition);

    return true;
}

void GenSFen::SearchIteration(Board& board, Search& search) {
    // search.FixDepth(depth);
    search.FixNodes(m_config.FIXED_NODES);
    search.IterativeDeepening(board);
}

bool GenSFen::NoMoves(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return moves.empty();
}

Move GenSFen::RandomMove(Board& board, Utils::PRNG& rng) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return MoveGenerator::RandomMove(moves, rng);
}

MoveList GenSFen::SortGoodMoves(Board& board, Search & search) {
    MoveList allMoves = MoveGenerator::GenerateMoves(board);
    MoveList goodMoves;

    Sorting::SortMoves(board, allMoves, Hash::tt, search.m_heuristics, 0);

    for(const Move move : allMoves) {
        if(!Scorer::IsNegativeCapture(move.Score())) {
            goodMoves.add(move);
        }
    }

    return goodMoves;
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

bool GenSFen::WriteRunMetadata(const std::string& mode, int concurrency) const {
    const std::filesystem::path metadataPath = std::filesystem::path(m_config.outputDir) / "run_metadata.txt";

    std::ofstream metadata(metadataPath);
    if(!metadata.is_open())
        return false;

    // Commands
    metadata << "mode=" << mode << '\n';
    metadata << "nodes=" << m_config.FIXED_NODES << '\n';
    metadata << "max_games=" << (m_maxGames == INFINITE ? "infinite" : std::to_string(m_maxGames)) << '\n';
    metadata << "seed=" << m_config.seed << '\n';
    metadata << "concurrency=" << concurrency << '\n';
    metadata << '\n';

    // Metadata
    metadata << "timestamp_unix=" << static_cast<long long>(std::time(nullptr)) << '\n';
    metadata << '\n';
    
    // Input data
    metadata << "# Input data" << '\n';
    metadata << "book_file=" << (mode == "games" ? m_config.bookFile : "-") << '\n';
    metadata << '\n';

    // Behavior
    metadata << "# Behavior" << '\n';
    metadata << "soft_randomize_plies=" << m_config.SOFT_RANDOMIZE_PLIES << '\n';
    metadata << '\n';

    // Adjudication
    metadata << "# Adjudication" << '\n';
    metadata << "adj_threshold_win=" << m_config.ADJUDICATION_THRESHOLD_WIN << '\n';
    metadata << "adj_threshold_draw=" << m_config.ADJUDICATION_THRESHOLD_DRAW << '\n';
    metadata << "adj_plies_win=" << m_config.ADJUDICATION_PLIES_WIN << '\n';
    metadata << "adj_plies_draw=" << m_config.ADJUDICATION_PLIES_DRAW << '\n';
    metadata << "adj_max_plies=" << m_config.MAX_PLIES << '\n';
    metadata << '\n';

    // Threads
    metadata << "# Threads" << '\n';
    if(mode == "games" || mode == "random") {
        for(int i = 0; i < concurrency; i++) {
            metadata << "thread_" << i << ".seed=" << SeedForThread(i)
                     << " file=evals_generated_" << (i + 1) << ".epd\n";
        }
    }

    return true;
}
