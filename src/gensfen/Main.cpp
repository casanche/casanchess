#include "Uci.h"
#include "gensfen/GenSFen.h"

#include "Attacks.h"
#include "Evaluation.h"
#include "ZobristKeys.h"

#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {
    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();

    UCI_CLASSICAL_EVAL = true;

    std::string mode;
    int concurrency = 0;

    int opt;
    while( (opt = getopt(argc, argv, "m:c:")) != -1 ) {
        switch(opt) {
            //Mode (required): 'games', 'random' or 'random_benchmark'
            case 'm': {
                mode = optarg;
            }
            //Concurrency (optional). Default: max_threads - 1
            case 'c': {
                concurrency = std::atoi(optarg);
            }
            default: break;
        }
    }

    GenSFen gensfen;
    gensfen.Run(mode, concurrency);

    return 0;
}
