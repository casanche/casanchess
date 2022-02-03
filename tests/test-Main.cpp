#include "test-Common.h"

int main(int argc, char** argv) {
    TestCommon::InitEngine();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


TestCommon::EPDPosition TestCommon::ReadEPDLine(std::string line) {
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