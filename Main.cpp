#include "Attacks.h"
#include "Evaluation.h"
#include "Interface.h"
#include "Uci.h"
#include "Utils.h"
#include "ZobristKeys.h"
#include "GenSFen.h"

#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {
    Utils::Clock clock;
    clock.Start();

    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();

    std::string gensfen_mode;

    int opt;
    while( (opt = getopt(argc, argv, "ijg:")) != -1 ) {
        switch(opt) {
            case 'i': { //Interface
                Interface interface;
                interface.Start();
                return 0;
            }
            case 'j': { //Interface with testing position
                Interface interface;
                interface.Start("2r1rbk1/p1p2pp1/1p4p1/n1P3P1/3Pq3/P3B2P/R3BP2/3QR1K1 w - - 1 25");
                return 0;
            }
            case 'g': { //GenSFen mode: 'games', 'random' or 'random_benchmark'
                GenSFen gensfen;
                gensfen.Run(optarg);
                return 0;
            }
            default: break;
        }
    }

    Uci uci;
    uci.Launch();

    int elapsed = clock.Elapsed();
    std::cout << "[TIME] " << elapsed << " ms" << std::endl;

    return 0;
}
