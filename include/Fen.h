#ifndef FEN_H
#define FEN_H

class Board;
#include <string>
#include <map>

typedef std::map<std::string, std::string> EPDLine;

class Fen {
public:
    static void SetPosition(Board& board, std::string fenString);

    static EPDLine ReadEPDLine(const std::string& line);
};

#endif //FEN_H
