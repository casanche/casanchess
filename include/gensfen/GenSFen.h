#ifndef GENSFEN_H
#define GENSFEN_H

#include "Board.h"
#include "Search.h"
#include "Utils.h"

#include <fstream>
#include <string>
#include <vector>

typedef std::vector<std::string> BookPositions;
struct CurrentPosition;

class GenSFen {
public:
    GenSFen();

    void Run(const std::string& gensfen_mode);
    
private:
    void Games(std::string filename);
    void Random(std::string filename);
    void RandomBenchmark(int maxGames);

    int GenerateRandomPosition(Board& board, std::string& position);
    void WriteEvals(Board& board, Search& search, std::ofstream& outputFile, CurrentPosition& currentPosition,
                    uint thresholdEval, uint thresholdEvalBoth, uint minPly = 0);

    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board);

    Utils::PRNG m_rng;
};

#endif //GENSFEN_H