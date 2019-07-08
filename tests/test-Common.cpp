#include "test-Common.h"

Test::EPDPosition Test::ReadEPDLine(std::string line) {
    std::string fen, temp, bestMove, id;

    std::istringstream stream(line);
    stream >> fen;
    stream >> temp; fen += " " + temp;
    stream >> temp; fen += " " + temp;
    stream >> temp; fen += " " + temp;
    stream >> temp;
    if(temp == "bm") {
        stream >> bestMove;
        while(bestMove.back() != ';') {
            stream >> temp; bestMove += " " + temp;
        }
        bestMove.pop_back();
    }
    stream >> temp;
    if(temp == "id") {
        stream >> id;
        while(id.back() != ';') {
            stream >> temp; id += " " + temp;
        }
        id.pop_back();
    }

    EPDPosition pos;
    pos.fen = fen;
    pos.bestMove = bestMove;
    pos.id = id;
    return pos;
}
