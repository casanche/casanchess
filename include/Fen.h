#ifndef _FEN_H
#define _FEN_H

#include <string>

class Board;

class Fen {
public:
    static void SetPosition(Board& board, std::string fenString);
};

#endif //_FEN_H