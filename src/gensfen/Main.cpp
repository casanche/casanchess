#include "gensfen/GenSFen.h"

#include "Attacks.h"
#include "Evaluation.h"
#include "NNUE.h"
#include "Syzygy.h"
#include "Uci.h"
#include "ZobristKeys.h"

#include <iostream>
#include <span>
#include <string_view>

int main(int argc, char** argv) {
    Attacks::Init();
    Evaluation::Init(); //after Attacks
    Syzygy::Init(Syzygy::DEFAULT_PATH);
    ZobristKeys::Init();
    nnue.Load();

    UCI_CLASSICAL_EVAL = false;

    std::string mode;
    int concurrency = 0;

    // m: mode: ['games', 'random', or 'benchmark']
    // c: concurrency
    std::span<char*> args(argv, argc);
    for (size_t i = 1; i < args.size(); ++i) {
        std::string_view arg = args[i];
        if (arg == "-m" && i + 1 < args.size()) {
            mode = args[i + 1];
            ++i;
        } else if (arg == "-c" && i + 1 < args.size()) {
            concurrency = std::atoi(args[i + 1]);
            ++i;
        }
    }

    GenSFen gensfen;
    gensfen.Run(mode, concurrency);

    Syzygy::Free();

    return 0;
}
