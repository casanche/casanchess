#include "Syzygy.h"
#include "fathom/src/tbprobe.h"

#include "Board.h"
#include "Uci.h"

#include <iostream>

unsigned int Syzygy::Init(const std::string& path) {
    tb_init( path.c_str() );
    if(TB_LARGEST)
        std::cout << "info string Syzygy initialized: " << TB_LARGEST << std::endl;
    return TB_LARGEST;
}

void Syzygy::Free() {
    tb_free();
}

bool Syzygy::Probe_WDL(Board &board, TB_RESULT &tb_result) {
    // Probe only after a 50-move rule reset
    if(board.FiftyRule() != 0) {
        return false;
    }
    // Don't probe if there are more pieces than system's TB (TB_LARGEST) or lower UCI limit
    int probeLimit = std::min(UCI_SYZYGY_PROBE_LIMIT, TB_LARGEST);
    if(PopCount(board.AllPieces()) > probeLimit) {
        return false;
    }

    uint result = tb_probe_wdl(
        board.GetPieces(WHITE, ALL_PIECES),
        board.GetPieces(BLACK, ALL_PIECES),
        board.GetPieces(WHITE, KING) | board.GetPieces(BLACK, KING),
        board.GetPieces(WHITE, QUEEN) | board.GetPieces(BLACK, QUEEN),
        board.GetPieces(WHITE, ROOK) | board.GetPieces(BLACK, ROOK),
        board.GetPieces(WHITE, BISHOP) | board.GetPieces(BLACK, BISHOP),
        board.GetPieces(WHITE, KNIGHT) | board.GetPieces(BLACK, KNIGHT),
        board.GetPieces(WHITE, PAWN) | board.GetPieces(BLACK, PAWN),
        board.FiftyRule(),
        board.CastlingRights(),
        board.EnPassantSquare() ? BitscanForward(board.EnPassantSquare()) : 0,
        board.ActivePlayer() == WHITE ? true : false
    );

    if(result == TB_RESULT_FAILED) {
        return false;
    }

    // Fill tb_result
    if(result == TB_WIN) {
        tb_result = TB_RESULT::WIN;
    } else if(result == TB_LOSS) {
        tb_result = TB_RESULT::LOSS;
    } else {
        tb_result = TB_RESULT::DRAW;
    }

    return true;
}
