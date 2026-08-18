#include "gensfen/GenSFen.h"
#include "gensfen/RandomPosition.h"

#include "BitboardUtils.h"
#include "Constants.h"
#include "MoveGenerator.h"
#include "Uci.h"

#include <filesystem>
#include <format>
#include <syncstream>
#include <sstream>
#include <thread>

GenSFen::GenSFen(GenSFenConfig config) : m_config(std::move(config)) {
    if(m_config.outputDir.empty())
        m_config.outputDir = "gensfen_output";
    if(m_config.bookFile.empty())
        m_config.bookFile = "bookfile.epd";

    UCI_OUTPUT = false;
}

void GenSFen::Run(const std::string& gensfen_mode, int concurrency, int nodes, int maxGames) {
    std::vector<std::thread> threads;

    if(m_config.seed == 0) {
        Utils::PRNG rng(0);
        m_config.seed = rng.Random64();
    }

    m_maxGames = maxGames == 0 ? INFINITE : maxGames;
    if(nodes > 0) m_config.FIXED_NODES = nodes;
    if(!concurrency) concurrency = std::thread::hardware_concurrency() - 1;
    
    // Timestamped directory
    std::filesystem::path timestampPath(m_config.outputDir);
    timestampPath /= std::format("{}/{}_{:%Y%m%d}", gensfen_mode, gensfen_mode, Utils::Clock::Now());
    std::filesystem::create_directories(timestampPath);
    m_config.outputDir = timestampPath.string();

    std::cout << "Starting GenSFen | Mode: " << gensfen_mode 
              << " | Concurrency: " << concurrency 
              << " | Output: " << m_config.outputDir << std::endl;

    if(!WriteRunMetadata(gensfen_mode, concurrency)) {
        std::cerr << "Error: Cannot write run metadata." << std::endl;
        return;
    }

    void (GenSFen::*function)(const std::string&, int) = nullptr;
    if(gensfen_mode == "games") function = &GenSFen::Games;
    // else if(gensfen_mode == "random") function = &GenSFen::Random;
    // else if(gensfen_mode == "benchmark") { RandomBenchmark(m_maxGames == INFINITE ? 100 : m_maxGames); return; }

    for(int i = 0; i < concurrency; i++) {
        std::string filename = m_config.outputDir + "/evals_generated_" + std::to_string(i + 1) + ".epd";
        threads.push_back(std::thread(function, this, filename, i));
    }
    for(auto& th : threads) th.join();
}

void GenSFen::Games(const std::string& filename, int threadIndex) {
    std::ofstream outputFile(filename);
    BookPositions bookPositions = ReadBook(m_config.bookFile);
    
    TT tt;
    Search search(tt);
    Board board;
    Utils::PRNG rng(m_config.seed + threadIndex);

    while(m_gamesPlayed.fetch_add(1) < m_maxGames) {
        int n_game = m_gamesPlayed.load();

        tt.Clear();
        search.ClearSearch(true);

        std::string startingFen = bookPositions[rng.Random32(0, static_cast<uint32_t>(bookPositions.size()) - 1)];
        board.SetFen(startingFen);

        const uint initialPly = board.Ply();

        std::vector<SavedPosition> savedPositions;
        savedPositions.reserve(512);

        int gameResult = 0; // 1 (Blanco gana), 0 (Empate), -1 (Negro gana)
        int adjWinCounter = 0, adjDrawCounter = 0;
        bool exitGame = false;

        while(!exitGame) {
            COLOR color = board.ActivePlayer();
            int playedPlies = board.Ply() - initialPly;
            bool softRandomize = playedPlies < m_config.SOFT_RANDOMIZE_PLIES;

            SearchIteration(board, search);
            int score = search.BestScore();
            Move bestMove = search.BestMove();
            Move nextMove = bestMove;

            // Soft-randomization
            if(softRandomize) {
                MoveList moves = SortGoodMoves(board, search);
                if(moves.size() >= 3)
                    nextMove = moves[rng.Random32(0, 2)];
            } else {
                SaveEvals(board, search, savedPositions);
            }

            board.MakeMove(nextMove);

            if(NoMoves(board)) {
                gameResult = board.IsCheck() ? (color == WHITE ? -1 : 1) : 0;
                exitGame = true; break;
            }
            if(board.FiftyRule() >= 100 || board.IsRepetitionDraw()) {
                gameResult = 0; exitGame = true; break;
            }

            // 3. Adjudicación de Victoria/Derrota
            int scoreWhitePOV = (color == WHITE) ? score : -score;
            if(std::abs(score) >= m_config.ADJUDICATION_THRESHOLD_WIN) {
                if(++adjWinCounter >= m_config.ADJUDICATION_PLIES_WIN) {
                    gameResult = scoreWhitePOV > 0 ? 1 : -1;
                    exitGame = true; break;
                }
            } else adjWinCounter = 0;

            // 4. Adjudicación de Empate
            if(std::abs(score) <= m_config.ADJUDICATION_THRESHOLD_DRAW) {
                if(++adjDrawCounter >= m_config.ADJUDICATION_PLIES_DRAW) {
                    gameResult = 0; exitGame = true; break;
                }
            } else adjDrawCounter = 0;

            // 5. Partida demasiado larga
            if(board.Ply() > m_config.ADJUDICATION_MAX_PLIES) {
                gameResult = 0; exitGame = true; break;
            }
        } // Game finished

        // Collect game global metrics
        int gameLength = board.Ply();
        int pawnCount = PopCount(board.Piece(WHITE, PAWN) | board.Piece(BLACK, PAWN));
        int heavyPiecesCount = PopCount(board.AllPieces()) - pawnCount - 2;

        // Write the batch
        std::stringstream ss;
        for(const auto& sp : savedPositions) {
            ss << sp.fen << ";bm " << sp.bestMove << ";ev " << sp.eval 
               << ";r " << gameResult << ";l " << gameLength 
               << ";pC " << pawnCount << ";PC " << heavyPiecesCount << "\n";
        }
        outputFile << ss.view(); // write to file

        // Logs to standard output
        std::osyncstream syncedLog(std::cout);
        syncedLog << "Thread: " << threadIndex << " | Game: " << n_game 
                  << " | Result: " << gameResult << " | Length: " << gameLength 
                  << " | Saved: " << savedPositions.size() << "\n";
    }
}

// void GenSFen::Random(std::string filename) {
//     std::ofstream outputFile;
//     outputFile.open(filename);

//     TT tt;
//     Search search(tt);
//     Board board;
//     State state;

//     const int maxGames = INFINITE;
//     for(int n_game = 0; n_game < maxGames; n_game++) {
//         std::string position;
//         GenerateRandomPosition(board, position);

//         //To avoid overlap in standard output due to multiple threads
//         std::stringstream ss;
//         ss << "thread: " << std::this_thread::get_id()
//            << " random position " << std::to_string(n_game)
//            << ", " << position << ", ";

//         //Game loop
//         state.NewGame();
//         bool exitGame;
//         do {
//             CurrentPosition currentPosition;
//             WriteEvals(board, search, outputFile, currentPosition, 100, 250, 0);

//             board.MakeMove(currentPosition.bestMove);

//             state.UpdateGame(currentPosition.evalPass, currentPosition.evalFail);

//             int nPieces = PopCount(board.AllPieces());
//             exitGame = (NoMoves(board) || nPieces <= 6 || board.Ply() > 40 || board.IsRepetitionDraw() || state.consecutiveFailedEvals >= 6);
//         } while(!exitGame);

//         state.FinishGame();
//         ss << "writtenEvals: " << state.gameWrittenEvals
//            << ", consecutive fails: " << state.consecutiveFailedEvals
//            << ", totalWrittenEvals: " << state.totalWrittenEvals
//            << std::endl;
//         std::cout << ss.str();

//     } //games

//     outputFile.close();
// }

// void GenSFen::RandomBenchmark(int maxGames) {
//     Utils::Clock clock, global_clock;
//     global_clock.Start();
//     int64_t time_choose = 0;
//     int64_t time_search = 0;
//     int passed = 0;

//     TT tt;
//     Search search(tt);
//     Board board;
//     UCI_Limits limits = UCI_Limits::FixDepth(7);

//     for(int n_game = 1; n_game <= maxGames; n_game++) {
//         clock.Start();
//         std::string position;
//         int tries = GenerateRandomPosition(board, position);
//         time_choose = clock.Elapsed();

//         clock.Start();

//         search.IterativeDeepening(board, limits);
//         int score_active = search.BestScore();

//         board.MakeNull();
//         search.IterativeDeepening(board, limits);
//         int score_inactive = search.BestScore();
//         board.TakeNull();

//         time_search = clock.Elapsed();

//         bool evalPass = (abs(score_active) < 100) || (abs(score_inactive) < 100);
//         bool evalPassBoth = abs(score_active) < 250 && abs(score_inactive) < 250;
//         if(evalPass && evalPassBoth) {
//             passed++;
//         }

//         P("game " << n_game << " tries " << tries
//                   << " time_choose " << time_choose
//                   << " time_search " << time_search);

//     } //games

//     P("passed: " << passed << " total time " << global_clock.Elapsed() );
// }

// int GenSFen::GenerateRandomPosition(Board& board, std::string& position) {
//     TT tt;
//     Search search_depth1(tt);
//     RandomPositionGenerator randomPosGen;

//     bool validScore = false;
//     int tries = 0;

//     do {
//         position = randomPosGen.GenerateV2(board);
//         tries++;

//         int nPieces = PopCount(board.AllPieces());
//         if(NoMoves(board) || nPieces <= 6)
//             continue;
//         search_depth1.IterativeDeepening(board, UCI_Limits::FixDepth(1));
//         int score_active = search_depth1.BestScore();

//         board.MakeNull();
//         if(NoMoves(board))
//             continue;
//         search_depth1.IterativeDeepening(board, UCI_Limits::FixDepth(1));
//         int score_inactive = search_depth1.BestScore();

//         bool evalPass = (abs(score_active) < 66) || (abs(score_inactive) < 66);
//         bool evalPassBoth = abs(score_active) < 150 && abs(score_inactive) < 150;
//         validScore = evalPass && evalPassBoth;
//         if(validScore)
//             board.TakeNull();
//     } while(!validScore);

//     return tries;
// }

bool GenSFen::NoMoves(Board& board) {
    return MoveGenerator::GenerateMoves(board).empty();
}

Move GenSFen::RandomMove(Board& board) {
    MoveList moves = MoveGenerator::GenerateMoves(board);
    return MoveGenerator::RandomMove(moves);
}

BookPositions GenSFen::ReadBook(const std::string& bookPath) {
    std::ifstream bookFile(bookPath);

    BookPositions bookPositions;
    if(!bookFile.is_open()) {
        std::cerr << "Error: Cannot open book file: " << bookPath << std::endl;
        return bookPositions;
    }

    std::string position;
    while( std::getline(bookFile, position) ) {
        if(!position.empty())
            bookPositions.push_back(position);
    }
    return bookPositions;
}

// Filtra malas jugadas (capturas negativas) para la soft-randomization
MoveList GenSFen::SortGoodMoves(Board& board, Search& search) {
    MoveList allMoves = MoveGenerator::GenerateMoves(board);
    MoveList goodMoves;

    Move hashMove;
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

// Guardar evaluación con filtros (evitar mates, posiciones tácticas inestables, etc.)
bool GenSFen::SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions) {
    if(board.IsCheck() || !search.BestMove().IsQuiet() || IsWinValue(search.BestScore()))
        return false;

    int score = search.BestScore();
    int scoreWhitePOV = board.ActivePlayer() == WHITE ? score : -score;

    SavedPosition sp;
    sp.fen = board.GetFen();
    sp.bestMove = search.BestMove().Notation();
    sp.eval = scoreWhitePOV;
    savedPositions.push_back(sp);

    return true;
}

void GenSFen::SearchIteration(Board& board, Search& search) {
    search.IterativeDeepening(board, UCI_Limits::FixNodes(m_config.FIXED_NODES));
}

bool GenSFen::WriteRunMetadata(const std::string& mode, int concurrency) const {
    const std::filesystem::path metadataPath = std::filesystem::path(m_config.outputDir) / "run_metadata.txt";
    std::ofstream metadata(metadataPath);
    
    if(!metadata.is_open())
        return false;

    metadata << "mode=" << mode << '\n'
             << "nodes=" << m_config.FIXED_NODES << '\n'
             << "max_games=" << (m_maxGames == INFINITE ? "infinite" : std::to_string(m_maxGames)) << '\n'
             << "seed=" << m_config.seed << '\n'
             << "concurrency=" << concurrency << '\n'
             << "timestamp_unix=" << static_cast<long long>(std::time(nullptr)) << "\n\n"
             << "# Input data\n"
             << "book_file=" << (mode == "games" ? m_config.bookFile : "-") << "\n\n"
             << "# Behavior\n"
             << "soft_randomize_plies=" << m_config.SOFT_RANDOMIZE_PLIES << "\n\n"
             << "# Adjudication\n"
             << "adj_threshold_win=" << m_config.ADJUDICATION_THRESHOLD_WIN << '\n'
             << "adj_threshold_draw=" << m_config.ADJUDICATION_THRESHOLD_DRAW << '\n'
             << "adj_plies_win=" << m_config.ADJUDICATION_PLIES_WIN << '\n'
             << "adj_plies_draw=" << m_config.ADJUDICATION_PLIES_DRAW << '\n'
             << "adj_max_plies=" << m_config.ADJUDICATION_MAX_PLIES << "\n\n"
             << "# Threads\n";

    if(mode == "games" || mode == "random") {
        for(int i = 0; i < concurrency; i++) {
            metadata << "thread_" << i << ".seed=" << m_config.seed + i
                     << " file=evals_generated_" << (i + 1) << ".epd\n";
        }
    }

    return true;
}
