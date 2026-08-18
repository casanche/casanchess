#pragma once

#include "Board.h"
#include "Search.h"
#include "Utils.h"

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
    uint ADJUDICATION_MAX_PLIES = 400;
};

struct SavedPosition {
    std::string fen;
    std::string bestMove;
    int eval;
};

class GenSFen {
public:
    explicit GenSFen(GenSFenConfig config);

    void Run(const std::string& gensfen_mode, int concurrency, int nodes, int maxGames);
    
private:
    void Games(const std::string& filename, int threadIndex);
    // void Random(const std::string& filename, int threadIndex);
    // void RandomBenchmark(int maxGames);

    // int GenerateRandomPosition(Board& board, std::string& position);

    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board);

    ///
    MoveList SortGoodMoves(Board& board, Search& search);
    bool SaveEvals(Board& board, Search& search, std::vector<SavedPosition>& savedPositions);
    void SearchIteration(Board& board, Search& search);

    bool WriteRunMetadata(const std::string& mode, int concurrency) const;

    GenSFenConfig m_config = {};
    int m_maxGames = 0;

    std::atomic<int> m_gamesPlayed{0};
};
