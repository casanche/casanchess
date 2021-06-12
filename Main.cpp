#include "Attacks.h"
#include "Board.h"
#include "Interface.h"
#include "Uci.h"
#include "Utils.h"

#include <iostream>

int main(int argc, char** argv) {
    Utils::Clock clock;
    clock.Start();

    //Testing position
    if(argc > 1 && argv[1] == std::string("-j")) {
        Interface interface;
        interface.Start("2r1rbk1/p1p2pp1/1p4p1/n1P3P1/3Pq3/P3B2P/R3BP2/3QR1K1 w - - 1 25");
        return 0;
    }

    //Starting position
    if(argc > 1 && argv[1] == std::string("-i")) {
        Interface interface;
        interface.Start();
        return 0;
    }

    Uci uci;
    uci.Launch();

    int elapsed = clock.Elapsed();
    std::cout << "[TIME] " << elapsed << " ms" << std::endl;

    return 0;
}
