#include "Uci.h"
#include "gensfen/GenSFen.h"

#include "Attacks.h"
#include "Evaluation.h"
#include "ZobristKeys.h"

#include <iostream>
#include <span>
#include <string_view>

int main(int argc, char** argv) {
    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();

    UCI_CLASSICAL_EVAL = true;

    std::string mode;
    int concurrency = 0;

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

    return 0;
}
