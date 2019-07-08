#include "Attacks.h"
#include "Board.h"
#include "Interface.h"
#include "Uci.h"

#include <iostream>
#include <chrono>

int main(int argc, char** argv) {
    typedef std::chrono::high_resolution_clock Clock;
    auto t0 = Clock::now();

    //Testing position
    if(argc > 1 && argv[1] == std::string("-j")) {
        Interface interface;
        interface.Start("r2Nqb1r/pQ1bp1pp/1pn1p3/1k1p4/2p2B2/2P5/PPP2PPP/R3KB1R w - - 1 0");
        return 0;
    }

    //Starting position
    if(argc > 1 && argv[1] == std::string("-i")) {
        Interface interface;
        interface.Start();
        return 0;
    }

    //Perft
    std::string FEN = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    if(argc > 1 && argv[1] == std::string("-perft")) {
        int depth = std::atoi(argv[2]);

        Board board;
        board.SetFen(FEN);
        board.Print();
        P( board.Perft(depth) );

        for(auto elem : countMap) {
            P(elem.first << " " << elem.second);
        }
    }

    //Divide
    if(argc > 1 && argv[1] == std::string("-divide")) {
        int depth = std::atoi(argv[2]);

        Board board;
        board.SetFen(FEN);
        board.Print();
        board.Divide(depth);
    }

    Uci uci;
    uci.Launch();

    #ifdef DEBUG2
    std::cout << "[DEBUG] " << "sizeof(board): " << sizeof(board) / 8 << " bitboards" << std::endl;
    std::cout << "[DEBUG] " << "sizeof(generator): " << sizeof(generator) / 8 << " bitboards" << std::endl;
    #endif

    auto t1 = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0);
    std::cout << "[DEBUG] " << duration.count() << " ms" << std::endl;

    return 0;
}
