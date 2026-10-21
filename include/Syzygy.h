#pragma once

#include <string>
class Board;

namespace Syzygy {
    const std::string DEFAULT_PATH = "../data/syzygy/";
    enum class TB_RESULT { LOSS, DRAW, WIN };

    unsigned int Init(const std::string& path);
    void Free();

    // bool ProbeRoot(Board &board);
    bool Probe_WDL(const Board& board, TB_RESULT& tb_result);
}
