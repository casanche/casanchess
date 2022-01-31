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
    void RandomBenchmark();

    void WriteEvals(Board& board, Search& search, CurrentPosition& currentPosition, std::ofstream& outputFile);

    bool NoMoves(Board& board);
    BookPositions ReadBook(const std::string& bookPath);
    Move RandomMove(Board& board);

    bool m_createDebugFile;
    Utils::PRNG m_rng;
};

struct CurrentPosition {
    Move bestMove = Move();
    int calculatedDepth = -1;
    int evalFail = 0;
    int evalPass = 0;
};

#endif //GENSFEN_H