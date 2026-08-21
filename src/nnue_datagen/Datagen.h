#pragma once

#include "Board.h"
#include "Search.h"
#include "Utils.h"

#include <atomic>
#include <fstream>
#include <string>
#include <vector>

using BookPositions = std::vector<std::string>;
struct CurrentPosition;
class RandomPositionGenerator;

struct DatagenConfig {
    std::string outputDir;
    std::string bookFile;
    uint64_t seed = 0;

    // Behavior
    int FIXED_NODES = 250000;
    int SOFT_RANDOMIZE_PLIES = 0;

    // Random generator
    int RANDOM_SCORE_FILTER = 250;
    int RANDOM_VALIDATION_NODES = 5000;

    // Adjudication configuration
    int ADJUDICATION_EVAL_WIN = 200;
    int ADJUDICATION_PLIES_WIN = 4;
    uint ADJUDICATION_THRESHOLD_PLIES_DRAW = 140;
    int ADJUDICATION_EVAL_DRAW = 5;
    int ADJUDICATION_PLIES_DRAW = 14;
    uint ADJUDICATION_MAX_PLIES = 260;
};

struct SavedPosition {
    std::string fen;
    std::string bestMove;
    int eval;
};

class Datagen {
public:
    explicit Datagen(DatagenConfig config);

    void Run(const std::string& datagen_mode, int concurrency, int maxGames);
    
private:
    void Games(const std::string& filename, int threadIndex);
    void Random(const std::string& filename, int threadIndex);
    void RandomBenchmark(int maxGames);

    int GenerateRandomPosition(Board& board, std::string& position,
                               RandomPositionGenerator& posGen, Search& validationSearch);
    bool ValidateRandomPosition(Board& board, Search& search);

    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board);
    MoveList SortFilteredMoves(Board& board, Search& search);

    bool SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions);
    void SearchIteration(Board& board, Search& search);
    bool WriteRunMetadata(const std::string& mode, int concurrency) const;

    DatagenConfig m_config = {};
    int m_maxGames = 0;

    BookPositions m_bookPositions;

    std::atomic<int> m_gamesPlayed{0};
};
