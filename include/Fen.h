#ifndef FEN_H
#define FEN_H

#include <map>
#include <string>

class Board;

typedef std::map<std::string, std::string> EPDLine;

class Fen {
public:
    static void SetPosition(Board& board, std::string fenString);
    static std::string SetRandomPosition(Board& board);

    static std::string GetSimplifiedFen(Board& board);

    static EPDLine ReadEPDLine(const std::string& line);
};

#endif //FEN_H
