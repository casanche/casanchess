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

    int opt;
    while( (opt = getopt(argc, argv, "m:")) != -1 ) {
        switch(opt) {
            case 'm': { //Mode: 'games', 'random' or 'random_benchmark'
                GenSFen gensfen;
                gensfen.Run(optarg);
            }
            default: break;
        }
    }

    return 0;
}
