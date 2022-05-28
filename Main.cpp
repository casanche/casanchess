#include "Attacks.h"
#include "Evaluation.h"
#include "Interface.h"
#include "NNUE.h"
#include "Uci.h"
#include "Utils.h"
#include "ZobristKeys.h"

#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {
    Utils::Clock clock;
    clock.Start();

    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();
    nnue.Load();

    int opt;
    while( (opt = getopt(argc, argv, "ij:n:")) != -1 ) {
        switch(opt) {
            case 'i': { //Interface
                Interface interface;
                interface.Start();
                return 0;
            }
            case 'j': { //Interface from fen position
                Interface interface;
                interface.Start(optarg);
                return 0;
            }
            case 'n': { //Path to .nnue file
                nnue.Load(optarg);
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
