#pragma once

#include <map>
#include <string>

class Board;

const std::string INITIAL_POSITION_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

using EPDLine = std::map<std::string, std::string>;

class Fen {
public:
    static void SetPosition(Board& board, std::string fenString);
    static std::string SetRandomPosition(Board& board);

    static std::string GetFen(const Board& board);
    static std::string GetSimplifiedFen(const Board& board);

    static EPDLine ReadEPDLine(const std::string& line);
};
