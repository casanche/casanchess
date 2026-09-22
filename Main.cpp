#include "Attacks.h"
#include "Interface.h"
#include "NNUE.h"
#include "Syzygy.h"
#include "Uci.h"
#include "Utils.h"
#include "ZobristKeys.h"
#include "src/nnue_embed/EmbeddedNetwork.h"

#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char** argv) {
    Utils::Clock clock;
    clock.Start();

    std::string networkPath;
    std::string_view mode = "uci";
    std::string fen;
    int benchDepth = 9;

    // Parse command line arguments
    for(int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if(arg == "-i") {
            mode = "interactive";
            if(i + 1 < argc && argv[i + 1][0] != '-')
                fen = argv[++i];
        }
        else if(arg == "-n" && i + 1 < argc) {
            networkPath = argv[++i];
        }
        else if(arg == "bench") {
            mode = "bench";
            if(i + 1 < argc && argv[i + 1][0] != '-')
                benchDepth = std::stoi(argv[++i]);
        }
    }

    // NNUE loading
    const auto embeddedNetwork = EmbeddedNetwork::Bytes();
    const bool loaded = networkPath.empty() ? NNUE::LoadBytes(embeddedNetwork)
                                            : NNUE::LoadFile(networkPath);
    if(!loaded)
        return 1;

    // Initialize components
    Attacks::Init();
    Syzygy::Init(Syzygy::DEFAULT_PATH);
    ZobristKeys::Init();

    if(mode == "interactive") {
        Interface interface;
        interface.Start(fen);

        Syzygy::Free();
        return 0;
    }

    Uci uci(embeddedNetwork);

    if(mode == "bench") {
        uci.Bench(benchDepth, false);
    }
    else {
        uci.Launch();
        std::cout << "info string [TIME] " << clock.Elapsed() << " ms" << std::endl;
    }

    Syzygy::Free();
    return 0;
}
