#include "Attacks.h"
#include "Evaluation.h"
#include "Interface.h"
#include "NNUE.h"
#include "Syzygy.h"
#include "Uci.h"
#include "Utils.h"
#include "ZobristKeys.h"

#include <iostream>
#include <span>
#include <string_view>

int main(int argc, char** argv) {
    Utils::Clock clock;
    clock.Start();

    Attacks::Init();
    Evaluation::Init(); //after Attacks
    Syzygy::Init(Syzygy::DEFAULT_PATH);
    ZobristKeys::Init();
    NNUE::Load();

    // Parse command line arguments
    std::span<char*> args(argv, argc);
    for (size_t i = 1; i < args.size(); ++i) {
        std::string_view arg = args[i];

        if (arg == "-i") {
            Interface interface;
            interface.Start();
            return 0;
        }
        else if (arg == "-j" && i + 1 < args.size()) {
            // Interface from fen position
            Interface interface;
            interface.Start(args[i + 1]);
            return 0;
        }
        else if (arg == "-n" && i + 1 < args.size()) {
            // Path to .nnue file
            NNUE::Load(args[i + 1]);
            ++i; // Skip next argument as it's the path
        }
        else if (arg == "bench") {
            int depth = 9;
            if (i + 1 < args.size()) {
                try {
                    depth = std::stoi(args[i + 1]);
                    ++i;
                } catch (...) {}
            }
            Uci uci;
            uci.Bench(depth, false);
            return 0;
        }
    }

    Uci uci;
    uci.Launch();

    int64_t elapsed = clock.Elapsed();
    std::cout << "[TIME] " << elapsed << " ms" << std::endl;

    Syzygy::Free();

    return 0;
}
