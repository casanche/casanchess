#include "Datagen.h"
#include "RandomPosition.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <filesystem>
#include <format>
#include <cmath>
#include <syncstream>
#include <sstream>
#include <thread>

Datagen::Datagen(DatagenConfig config) : m_config(std::move(config)) {
    if(m_config.outputDir.empty())
        m_config.outputDir = "datagen_output";
    if(m_config.bookFile.empty())
        m_config.bookFile = "bookfile.epd";

    UCI_OUTPUT = false;
}

void Datagen::Run(const std::string& datagen_mode, int concurrency, int maxGames) {
    std::vector<std::thread> threads;

    if(datagen_mode != "games" && datagen_mode != "random" && datagen_mode != "benchmark") {
        std::cerr << "Error: Unsupported Datagen mode: " << datagen_mode << std::endl;
        return;
    }

    if(m_config.seed == 0) {
        Utils::PRNG rng(0);
        m_config.seed = rng.Random64();
    }

    m_maxGames = maxGames == 0 ? INFINITE : maxGames;
    m_gamesPlayed.store(0);
    if(concurrency <= 0) {
        const unsigned int hardwareThreads = std::thread::hardware_concurrency();
        concurrency = hardwareThreads > 1 ? static_cast<int>(hardwareThreads - 1) : 1;
    }

    if(datagen_mode == "benchmark") {
        RandomBenchmark(m_maxGames == INFINITE ? 100 : m_maxGames);
        return;
    }

    // Book
    if(datagen_mode == "games") {
        m_bookPositions = ReadBook(m_config.bookFile);
        if(m_bookPositions.empty()) {
            std::cerr << "Error: Book is empty or missing. Bookfile: " << m_config.bookFile << std::endl;
            return;
        }
    }

    // Timestamped directory
    std::filesystem::path timestampPath(m_config.outputDir);
    timestampPath /= std::format("{}/{}_{:%Y%m%d}", datagen_mode, datagen_mode, Utils::Clock::Now());
    std::filesystem::create_directories(timestampPath);
    m_config.outputDir = timestampPath.string();

    std::cout << "Starting Datagen | Mode: " << datagen_mode
              << " | Concurrency: " << concurrency 
              << " | Output: " << m_config.outputDir << std::endl;

    if(!WriteRunMetadata(datagen_mode, concurrency)) {
        std::cerr << "Error: Cannot write run metadata." << std::endl;
        return;
    }

    void (Datagen::*function)(const std::string&, int) = nullptr;
    if(datagen_mode == "games") function = &Datagen::Games;
    else if(datagen_mode == "random") function = &Datagen::Random;

    for(int i = 0; i < concurrency; i++) {
        std::string filename = m_config.outputDir + "/evals_generated_" + std::to_string(i + 1) + ".epd";
        threads.push_back(std::thread(function, this, filename, i));
    }
    for(auto& th : threads) th.join();
}

void Datagen::Games(const std::string& filename, int threadIndex) {
    std::ofstream outputFile(filename);
    
    TT tt;
    tt.SetSize(8);
    Search search(tt);
    Board board;
    Utils::PRNG rng(m_config.seed + threadIndex);

    std::vector<SavedPosition> savedPositions;
    savedPositions.reserve(512);
    std::stringstream ss;

    for(;;) {
        const int n_game = m_gamesPlayed.fetch_add(1) + 1;
        if(n_game > m_maxGames)
            break;

        search.ClearSearch(true);

        savedPositions.clear();
        ss.str("");
        ss.clear();

        const std::string& startingFen = m_bookPositions[rng.Random32(0, static_cast<uint32_t>(m_bookPositions.size()) - 1)];
        board.SetFen(startingFen);

        const uint initialPly = board.Ply();

        int gameResult = 0; // 1 (White wins), 0 (draw), -1 (Black wins)
        int adjWinCounter = 0, adjDrawCounter = 0;
        bool exitGame = false;

        while(!exitGame) {
            COLOR color = board.ActivePlayer();

            // Natural game endings
            if(NoMoves(board)) {
                if(board.IsCheck())
                    gameResult = color == WHITE ? -1 : 1;
                else
                    gameResult = 0;
                break;
            }
            if(board.FiftyRule() >= 100 || board.IsRepetitionDraw()) {
                gameResult = 0;
                break;
            }

            int playedPlies = board.Ply() - initialPly;
            
            SearchIteration(board, search);
            int score = search.BestScore();
            Move bestMove = search.BestMove();
            Move nextMove = bestMove;

            if(nextMove == Move())
                break;

            // Soft randomization
            if(playedPlies < m_config.SOFT_RANDOMIZE_PLIES) {
                MoveList moves = SortFilteredMoves(board, search);
                if(moves.size() >= 3)
                    nextMove = moves[rng.Random32(0, 2)];
            } else {
                SaveEvals(board, search, savedPositions);
            }

            board.MakeMove(nextMove);

            // Win/loss adjudication
            int scoreWhitePOV = (color == WHITE) ? score : -score;
            if(std::abs(score) >= m_config.ADJUDICATION_EVAL_WIN) {
                if(++adjWinCounter >= m_config.ADJUDICATION_PLIES_WIN) {
                    gameResult = scoreWhitePOV > 0 ? 1 : -1;
                    exitGame = true; break;
                }
            } else adjWinCounter = 0;

            // Draw adjudication
            if(board.Ply() >= m_config.ADJUDICATION_THRESHOLD_PLIES_DRAW &&
               std::abs(score) <= m_config.ADJUDICATION_EVAL_DRAW) {
                if(++adjDrawCounter >= m_config.ADJUDICATION_PLIES_DRAW) {
                    gameResult = 0; exitGame = true; break;
                }
            } else adjDrawCounter = 0;

            // Maximum game length
            if(board.Ply() > m_config.ADJUDICATION_MAX_PLIES) {
                gameResult = 0; exitGame = true; break;
            }
        } // Game finished

        // Collect game-level metrics
        int gameLength = board.Ply();
        int pawnCount = PopCount(board.Piece(WHITE, PAWN) | board.Piece(BLACK, PAWN));
        int heavyPiecesCount = PopCount(board.AllPieces()) - pawnCount - 2;

        // Write the batch
        for(const auto& sp : savedPositions) {
            ss << sp.fen << ";bm " << sp.bestMove << ";ev " << sp.eval 
               << ";r " << gameResult << ";l " << gameLength 
               << ";pC " << pawnCount << ";PC " << heavyPiecesCount << "\n";
        }
        outputFile << ss.view();

        // Log to standard output
        std::osyncstream syncedLog(std::cout);
        syncedLog << "Thread: " << threadIndex + 1 << " | Game: " << n_game 
                  << " | Result: " << gameResult << " | Length: " << gameLength 
                  << " | Saved: " << savedPositions.size() << "\n";
    }
}

void Datagen::Random(const std::string& filename, int threadIndex) {
    std::ofstream outputFile(filename);

    TT tt;
    tt.SetSize(8);
    Search search(tt);
    Search validationSearch(tt);
    Board board;

    Utils::PRNG rng(m_config.seed + threadIndex);
    RandomPositionGenerator randomPosGen(m_config.seed + threadIndex);

    std::vector<SavedPosition> savedPositions;
    savedPositions.reserve(512);
    std::stringstream ss;

    for(;;) {
        const int n_game = m_gamesPlayed.fetch_add(1) + 1;
        if(n_game > m_maxGames)
            break;

        search.ClearSearch(true);

        savedPositions.clear();
        ss.str("");
        ss.clear();

        std::string startingFen;
        GenerateRandomPosition(board, startingFen, randomPosGen, validationSearch);

        const uint initialPly = board.Ply();

        int gameResult = 0;
        int adjWinCounter = 0;
        int adjDrawCounter = 0;

        for(;;) {
            if(NoMoves(board)) {
                gameResult = board.IsCheck() ? (board.ActivePlayer() == WHITE ? -1 : 1) : 0;
                break;
            }
            if(board.FiftyRule() >= 100 || board.IsRepetitionDraw()) {
                break;
            }

            const COLOR color = board.ActivePlayer();

            SearchIteration(board, search);
            const int score = search.BestScore();
            Move nextMove = search.BestMove();
            if(nextMove == Move())
                break;

            SaveEvals(board, search, savedPositions);

            board.MakeMove(nextMove);

            // Adjudication
            const int scoreWhitePOV = color == WHITE ? score : -score;
            if(std::abs(score) >= m_config.ADJUDICATION_EVAL_WIN) {
                if(++adjWinCounter >= m_config.ADJUDICATION_PLIES_WIN) {
                    gameResult = scoreWhitePOV > 0 ? 1 : -1;
                    break;
                }
            } else {
                adjWinCounter = 0;
            }

            if(board.Ply() >= m_config.ADJUDICATION_THRESHOLD_PLIES_DRAW &&
               std::abs(score) <= m_config.ADJUDICATION_EVAL_DRAW) {
                if(++adjDrawCounter >= m_config.ADJUDICATION_PLIES_DRAW) {
                    break;
                }
            } else {
                adjDrawCounter = 0;
            }

            if(board.Ply() > m_config.ADJUDICATION_MAX_PLIES)
                break;
        }

        const int gameLength = board.Ply() - initialPly;
        const int pawnCount = PopCount(board.Piece(WHITE, PAWN) | board.Piece(BLACK, PAWN));
        const int heavyPiecesCount = PopCount(board.AllPieces()) - pawnCount - 2;

        for(const auto& sp : savedPositions) {
            ss << sp.fen << ";bm " << sp.bestMove << ";ev " << sp.eval
               << ";r " << gameResult << ";l " << gameLength
               << ";pC " << pawnCount << ";PC " << heavyPiecesCount << "\n";
        }
        outputFile << ss.view();

        std::osyncstream syncedLog(std::cout);
        syncedLog << "Thread: " << threadIndex + 1 << " | Random Game: " << n_game
                  << " | Result: " << gameResult << " | Length: " << gameLength
                  << " | Saved: " << savedPositions.size() << "\n";
    }
}

void Datagen::RandomBenchmark(int maxGames) {
    std::cout << "--- Random Benchmark Start (" << maxGames << " positions) ---" << std::endl;

    Utils::Clock clock;
    Utils::Clock globalClock;
    int64_t totalGenerationTime = 0;
    int64_t totalSearchTime = 0;
    int totalTries = 0;

    TT tt;
    tt.SetSize(16);
    Search validationSearch(tt);
    Board board;
    RandomPositionGenerator posGen(m_config.seed);

    for(int n_game = 1; n_game <= maxGames; ++n_game) {
        clock.Start();
        std::string position;
        totalTries += GenerateRandomPosition(board, position, posGen, validationSearch);
        totalGenerationTime += clock.Elapsed();

        clock.Start();
        validationSearch.ClearSearch(true);
        validationSearch.IterativeDeepening(board, UCI_Limits::FixDepth(7));
        totalSearchTime += clock.Elapsed();

        if(n_game % 10 == 0 || n_game == maxGames) {
            std::cout << "Benchmark: " << n_game << "/" << maxGames
                      << " (Average tries: " << totalTries / n_game << ")" << std::endl;
        }
    }

    std::cout << "--- Benchmark Results ---" << std::endl;
    std::cout << "Total Passed      : " << maxGames << std::endl;
    std::cout << "Time Generating   : " << totalGenerationTime << " ms" << std::endl;
    std::cout << "Time Searching d7 : " << totalSearchTime << " ms" << std::endl;
    std::cout << "Total Time        : " << globalClock.Elapsed() << " ms" << std::endl;
}

bool Datagen::NoMoves(Board& board) {
    return MoveGenerator::GenerateMoves(board).empty();
}

Move Datagen::RandomMove(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return MoveGenerator::RandomMove(moves);
}

BookPositions Datagen::ReadBook(const std::string& bookPath) {
    std::ifstream bookFile(bookPath);

    BookPositions bookPositions;
    if(!bookFile.is_open()) {
        std::cerr << "Error: Cannot open book file: " << bookPath << std::endl;
        return bookPositions;
    }

    std::string position;
    while(std::getline(bookFile, position)) {
        if(!position.empty() && position.back() == '\r')
            position.pop_back();
        if(!position.empty())
            bookPositions.push_back(position);
    }
    return bookPositions;
}

// Filter bad captures for soft-randomization
MoveList Datagen::SortFilteredMoves(Board& board, Search& search) {
    MoveList allMoves = MoveGenerator::GenerateMoves(board);
    MoveList goodMoves;

    Move hashMove = Move();
    TTEntry* ttEntry = search.m_tt.Probe(board.ZKey());
    if(ttEntry)
        hashMove = ttEntry->bestMove;

    Sorting::SortMoves(board, allMoves, hashMove, search.m_heuristics, 0);

    for(const Move move : allMoves) {
        if(!Scorer::IsNegativeCapture(move.Score())) {
            goodMoves.add(move);
        }
    }
    return goodMoves;
}

// Save positions with filters: quiets, no-check, no-mate
bool Datagen::SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions) {
    if(board.IsCheck() || !search.BestMove().IsQuiet() || IsWinValue(search.BestScore()))
        return false;

    int score = search.BestScore();
    int scoreWhitePOV = board.ActivePlayer() == WHITE ? score : -score;

    savedPositions.push_back( {board.GetFen(), search.BestMove().Notation(), scoreWhitePOV} );

    return true;
}

bool Datagen::ValidateRandomPosition(Board& board, Search& search) {
    const int nPieces = PopCount(board.AllPieces());
    if(NoMoves(board) || nPieces <= 6 || board.IsCheck())
        return false;

    search.ClearSearch(false);
    search.IterativeDeepening(board, UCI_Limits::FixNodes(m_config.RANDOM_VALIDATION_NODES));
    return std::abs(search.BestScore()) <= m_config.RANDOM_SCORE_FILTER;
}

int Datagen::GenerateRandomPosition(Board& board, std::string& position,
                                    RandomPositionGenerator& posGen, Search& validationSearch) {
    int tries = 0;

    do {
        position = posGen.GenerateV2(board);
        ++tries;
    } while(!ValidateRandomPosition(board, validationSearch));

    return tries;
}

void Datagen::SearchIteration(Board& board, Search& search) {
    search.IterativeDeepening(board, UCI_Limits::FixNodes(m_config.FIXED_NODES));
}

bool Datagen::WriteRunMetadata(const std::string& mode, int concurrency) const {
    const std::filesystem::path metadataPath = std::filesystem::path(m_config.outputDir) / "run_metadata.txt";
    std::ofstream metadata(metadataPath);
    
    if(!metadata.is_open())
        return false;

    metadata << "mode=" << mode << '\n'
             << "nodes=" << m_config.FIXED_NODES << '\n'
             << "max_games=" << (m_maxGames == INFINITE ? "infinite" : std::to_string(m_maxGames)) << '\n'
             << "seed=" << m_config.seed << '\n'
             << "concurrency=" << concurrency << '\n'
             << "draw_contempt=" << UCI_DRAW_CONTEMPT << '\n'
             << "timestamp_unix=" << static_cast<long long>(std::time(nullptr)) << "\n\n"
             << "# Input data\n"
             << "book_file=" << (mode == "games" ? m_config.bookFile : "-") << "\n\n"
             << "# Behavior\n"
             << "soft_randomize_plies=" << m_config.SOFT_RANDOMIZE_PLIES << '\n'
             << "random_score_filter=" << m_config.RANDOM_SCORE_FILTER << '\n'
             << "random_validation_nodes=" << m_config.RANDOM_VALIDATION_NODES << "\n\n"
             << "# Adjudication\n"
             << "adj_eval_win=" << m_config.ADJUDICATION_EVAL_WIN << '\n'
             << "adj_threshold_plies_draw=" << m_config.ADJUDICATION_THRESHOLD_PLIES_DRAW << '\n'
             << "adj_eval_draw=" << m_config.ADJUDICATION_EVAL_DRAW << '\n'
             << "adj_plies_win=" << m_config.ADJUDICATION_PLIES_WIN << '\n'
             << "adj_plies_draw=" << m_config.ADJUDICATION_PLIES_DRAW << '\n'
             << "adj_max_plies=" << m_config.ADJUDICATION_MAX_PLIES << "\n\n"
             << "# Threads\n";

    if(mode == "games" || mode == "random") {
        for(int i = 1; i <= concurrency; i++) {
            metadata << "thread_" << i << " file=evals_generated_" << i << ".epd\n";
        }
    }

    return true;
}
