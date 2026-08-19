#include "gensfen/GenSFen.h"

#include "Attacks.h"
#include "Evaluation.h"
#include "Syzygy.h"
#include "Uci.h"
#include "ZobristKeys.h"

#include <iostream>
#include <span>
#include <string_view>

struct CliArgs {
    std::string mode = "games";
    int concurrency = 0;
    int nodes = 250000;
    int maxGames = 0;
    std::string outputDir;
    std::string bookFile;
    uint64_t seed = 0;
    bool showHelp = false;
};

void PrintUsage() {
    std::cout << "Usage: gensfen -m <games|random|benchmark> [-c threads] [-n nodes]"
              << " [--max-games N] [-o output_dir] [-b book_file] [-s seed]\n";
}

bool ParseArgs(int argc, char** argv, CliArgs& argsOut) {
    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if(arg == "-m" && i + 1 < argc) argsOut.mode = argv[++i];
        else if(arg == "-c" && i + 1 < argc) argsOut.concurrency = std::atoi(argv[++i]);
        else if(arg == "-n" && i + 1 < argc) argsOut.nodes = std::atoi(argv[++i]);
        else if(arg == "--max-games" && i + 1 < argc) argsOut.maxGames = std::atoi(argv[++i]);
        else if((arg == "-o" || arg == "--output-dir") && i + 1 < argc) argsOut.outputDir = argv[++i];
        else if((arg == "-b" || arg == "--book-file") && i + 1 < argc) argsOut.bookFile = argv[++i];
        else if((arg == "-s" || arg == "--seed") && i + 1 < argc) argsOut.seed = std::strtoull(argv[++i], nullptr, 10);
        else if(arg == "-h" || arg == "--help") argsOut.showHelp = true;
        else {
            std::cerr << "Error: unknown argument: " << arg << std::endl;
            return false;
        }
    }

    if(argsOut.mode.empty() && !argsOut.showHelp) {
        std::cerr << "Error: missing required mode (-m).\n";
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    CliArgs args;
    if(!ParseArgs(argc, argv, args) || args.showHelp) {
        PrintUsage();
        return args.showHelp ? 0 : 1;
    }

    Attacks::Init();
    Evaluation::Init(); 
    Syzygy::Init(Syzygy::DEFAULT_PATH);
    ZobristKeys::Init();
    NNUE::Load();

    UCI_CLASSICAL_EVAL = false;

    GenSFenConfig config;
    config.outputDir = args.outputDir;
    config.bookFile = args.bookFile;
    config.seed = args.seed;

    GenSFen gensfen(config);
    gensfen.Run(args.mode, args.concurrency, args.nodes, args.maxGames);

    Syzygy::Free();
    return 0;
}
