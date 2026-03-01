#include "gensfen/GenSFen.h"

#include "Attacks.h"
#include "Evaluation.h"
#include "NNUE.h"
#include "Syzygy.h"
#include "Uci.h"
#include "ZobristKeys.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

struct CliArgs {
    std::string mode;
    int concurrency = 0;
    int depth = 7;
    int maxGames = 0;
    std::string outputDir;
    std::string bookFile;
    uint64_t seed = 0;
    bool showHelp = false;
};

void PrintUsage(std::ostream& os = std::cout) {
    os << "Usage: gensfen -m <games|random|benchmark> [-c threads] [-d depth]"
       << " [--max-games N] [-o output_dir] [-b book_file] [-s seed]" << std::endl;
}

bool ParseArgs(int argc, char** argv, CliArgs& argsOut) {
    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if(arg == "-m" && i + 1 < argc) {
            argsOut.mode = argv[++i];
        } else if(arg == "-c" && i + 1 < argc) {
            argsOut.concurrency = std::atoi(argv[++i]);
        } else if(arg == "-d" && i + 1 < argc) {
            argsOut.depth = std::atoi(argv[++i]);
        } else if(arg == "--max-games" && i + 1 < argc) {
            argsOut.maxGames = std::atoi(argv[++i]);
        } else if((arg == "-o" || arg == "--output-dir") && i + 1 < argc) {
            argsOut.outputDir = argv[++i];
        } else if((arg == "-b" || arg == "--book-file") && i + 1 < argc) {
            argsOut.bookFile = argv[++i];
        } else if((arg == "-s" || arg == "--seed") && i + 1 < argc) {
            argsOut.seed = std::strtoull(argv[++i], nullptr, 10);
        } else if(arg == "-h" || arg == "--help") {
            argsOut.showHelp = true;
        } else {
            std::cerr << "Error: unknown argument: " << arg << std::endl;
            PrintUsage(std::cerr);
            return false;
        }
    }

    return true;
}

bool ModifyArgs(CliArgs& args) {
    if(args.mode.empty()) {
        std::cerr << "Error: missing required mode (-m)." << std::endl;
        PrintUsage(std::cerr);
        return false;
    }

    if(args.outputDir.empty()) {
        if(const char* env = std::getenv("CASANCHESS_GENSFEN_OUTPUT_DIR"))
            args.outputDir = env;
        else
            args.outputDir = "gensfen-output";
    }
    if(args.bookFile.empty()) {
        if(const char* env = std::getenv("CASANCHESS_GENSFEN_BOOK_FILE"))
            args.bookFile = env;
    }

    if(args.mode == "games" && args.bookFile.empty()) {
        std::cerr << "Error: games mode requires a book file. "
                  << "Use -b/--book-file or CASANCHESS_GENSFEN_BOOK_FILE." << std::endl;
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char** argv) {
    CliArgs args;
    if(!ParseArgs(argc, argv, args))
        return 1;
    if(args.showHelp) {
        PrintUsage();
        return 0;
    }

    if(!ModifyArgs(args))
        return 1;

    Attacks::Init();
    Evaluation::Init(); //after Attacks
    Syzygy::Init(Syzygy::DEFAULT_PATH);
    ZobristKeys::Init();
    NNUE::Load();

    UCI_CLASSICAL_EVAL = false;

    GenSFenConfig config;
    config.outputDir = args.outputDir;
    config.bookFile = args.bookFile;
    config.seed = args.seed;

    GenSFen gensfen(config);
    gensfen.Run(args.mode, args.concurrency, args.depth, args.maxGames);

    Syzygy::Free();

    return 0;
}
