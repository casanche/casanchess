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

    auto t1 = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0);
    std::cout << "[DEBUG] " << duration.count() << " ms" << std::endl;

    return 0;
}
