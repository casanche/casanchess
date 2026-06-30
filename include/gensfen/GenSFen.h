#pragma once

#include "Board.h"
#include "Search.h"
#include "Utils.h"
#include "gensfen/RandomPosition.h"

#include <fstream>
#include <string>
#include <vector>

using BookPositions = std::vector<std::string>;
struct CurrentPosition;

struct GenSFenConfig {
    std::string outputDir;
    std::string bookFile;
    uint64_t seed = 0;
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
    bool WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition,
                    int thresholdEval, int thresholdEvalBoth, int tacticalThreshold, uint minPly = 0);

    bool DoRandomMove(Board& board, Utils::PRNG& rng);
    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board, Utils::PRNG& rng);

    int m_depth = 7;
    int m_maxGames = INFINITE;
    GenSFenConfig m_config;
};
