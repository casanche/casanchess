#ifndef FEN_H
#define FEN_H

#include <string>

class Board;

class Fen {
public:
    static void SetPosition(Board& board, std::string fenString);
};

#endif //FEN_H