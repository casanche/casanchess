#pragma once

#include "Board.h"
#include "Search.h"
#include "Utils.h"
#include "gensfen/RandomPosition.h"

#include <atomic>
#include <fstream>
#include <string>
#include <vector>

using BookPositions = std::vector<std::string>;
struct CurrentPosition;

struct GenSFenConfig {
    std::string outputDir;
    std::string bookFile;
    uint64_t seed = 0;

    // Behavior
    int FIXED_NODES = 250000;
    int SOFT_RANDOMIZE_PLIES = 4;

    // Adjudication configuration
    int ADJUDICATION_THRESHOLD_WIN = 200;
    int ADJUDICATION_THRESHOLD_DRAW = 10;
    int ADJUDICATION_PLIES_WIN = 6;
    int ADJUDICATION_PLIES_DRAW = 30;
    uint MAX_PLIES = 400;

    // int WRITE_EVALS_GAMES_SINGLE = 200;
    // int WRITE_EVALS_GAMES_BOTH = 800;
    int WRITE_EVALS_RANDOM_SINGLE = 160;
    int WRITE_EVALS_RANDOM_BOTH = 600;
};

struct SavedPosition {
    std::string fen = "";
    std::string bestMove = "";
    int eval = -INFINITE_SCORE;
};

class GenSFen {
public:
    explicit GenSFen(GenSFenConfig config);

    void Run(const std::string& gensfen_mode, int concurrency, int depth = 7, int maxGames = 0);
    
private:
    void Games(std::string filename, int threadIndex);
    void Random(std::string filename, int threadIndex);
    void RandomBenchmark(int maxGames);

    int GenerateRandomPosition(Board& board, std::string& position, RandomPositionGenerator& positionGenerator, Search& validationSearch);
    bool ValidateRandomPosition(Board& board, Search& search, int scoreFilter);
    
    bool SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions);
    void SearchIteration(Board& board, Search& search);

    bool DoRandomMove(Board& board, Utils::PRNG& rng);
    bool NoMoves(Board& board);
    MoveList SortGoodMoves(Board& board, Search& search);

    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board, Utils::PRNG& rng);

    uint64_t SeedForThread(int threadIndex) const;
    bool WriteRunMetadata(const std::string& mode, int concurrency) const;

    int m_depth = 10;
    int m_maxGames = INFINITE;
    GenSFenConfig m_config;

    std::atomic<int> m_gamesPlayed = 0;
};
