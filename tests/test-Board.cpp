#include "Board.h"
#include "MoveGenerator.h"
#include "test-Common.h"
using namespace TestCommon;


// ===========
// == Board ==
// ===========

TEST(FenTest, EnPassant) {
    Board board;
    board.SetFen("rnbqkbnr/p1pppppp/8/8/1Pp5/2N5/P2PPPPP/R1BQKBNR b KQkq b3");
    EXPECT_EQ(board.EnPassantSquare(), SquareBB(SQUARES::B3));
}

TEST(BoardTest, IsRepetitionDraw) {
    Board board;
    board.SetFen("8/p5pp/1r2bpk1/8/2P1P3/q1P2PQ1/PR4PP/2KR4 b - - 5 32");
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), false);
    board.MakeMove("h4g3"); EXPECT_EQ(board.IsRepetitionDraw(), true); //1st repetition
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("h4g3"); EXPECT_EQ(board.IsRepetitionDraw(), true); //2nd repetition
    board.MakeMove("g6h6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("g3h4"); EXPECT_EQ(board.IsRepetitionDraw(), true);
    board.MakeMove("h6g6"); EXPECT_EQ(board.IsRepetitionDraw(), true);
}

TEST(BoardTest, GivesCheck) {
    // Direct and discovered checks, promotions, en-passant and both castlings.
    for(const char* fen : {
        INITIAL_POSITION_FEN.c_str(),
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "4k3/8/3PB3/3N4/8/8/8/K2Q4 w - - 0 1",
        "4k3/8/8/8/8/8/4N3/K3R3 w - - 0 1",
        "k3r3/4n3/8/8/8/8/8/4K3 b - - 0 1",
        "8/8/8/8/8/8/R2K3k/8 w - - 0 1",
        "r3k2r/1P6/8/8/8/8/8/4K3 w kq - 0 1",
        "4k3/8/8/8/8/8/1p6/R3K2R b KQ - 0 1",
        "r7/1Pk5/8/8/8/8/8/7K w - - 0 1", // Knight and bishop promotion checks
        "8/8/8/k3pP1R/8/8/8/7K w - e6 0 1",
        "7k/8/8/8/K3Pp1r/8/8/8 b - e3 0 1",
        "5k2/8/8/8/8/8/8/R3K2R w KQ - 0 1",
        "3k4/8/8/8/8/8/8/R3K2R w KQ - 0 1"
    }) {
        SCOPED_TRACE(fen);
        Board board;
        board.SetFen(fen);

        for(Move move : MoveGenerator::GenerateMoves(board)) {
            SCOPED_TRACE(move.Notation());
            const bool givesCheck = board.GivesCheck(move);
            board.MakeMove(move, false);
            EXPECT_EQ(givesCheck, board.IsCheck());
            board.TakeMove(move);
        }
    }
}
