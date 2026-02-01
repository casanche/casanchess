#pragma once

#include "Board.h"
#include "Search.h"
#include "Utils.h"

#include <fstream>
#include <string>
#include <vector>

using BookPositions = std::vector<std::string>;
struct CurrentPosition;

class GenSFen {
public:
    GenSFen();

    void Run(const std::string& gensfen_mode, int concurrency, int depth = 7);
    
private:
    void Games(std::string filename);
    void Random(std::string filename);
    void RandomBenchmark(int maxGames);

    int GenerateRandomPosition(Board& board, std::string& position);
    bool ValidateRandomPosition(Board& board, Search& search, int scoreFilter);
    bool WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition,
                    int thresholdEval, int thresholdEvalBoth, int tacticalThreshold, uint minPly = 0);

    bool DoRandomMove(Board& board);
    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board);

    Utils::PRNG m_rng;
    int m_depth = 7;
};
