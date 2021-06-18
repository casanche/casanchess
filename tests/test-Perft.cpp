#include "test-Common.h"

#include "Board.h"
#include "Move.h"
#include "MoveGenerator.h"

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    TestCommon::InitEngine();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

//MoveGenerator
TEST(MoveGenerator, StartingPosition) {
    Board board;

    MoveGenerator generator;
    generator.GenerateMoves(board);
    MoveList moves = generator.Moves();

    EXPECT_EQ(moves.size(), 20U);
}

//https://www.chessprogramming.org/Perft_Results
TEST(Perft, StartingPosition) {
    Board board;
    // board.SetFen(STARTFEN);
    EXPECT_EQ(board.Perft(1), (u64)20);
    EXPECT_EQ(board.Perft(2), (u64)400);
    EXPECT_EQ(board.Perft(3), (u64)8902);
    EXPECT_EQ(board.Perft(4), (u64)197281);
    EXPECT_EQ(board.Perft(5), (u64)4865609);
}
TEST(Perft, Kiwipete) {
    Board board;
    board.SetFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    EXPECT_EQ(board.Perft(1), (u64)48);
    EXPECT_EQ(board.Perft(2), (u64)2039);
    EXPECT_EQ(board.Perft(3), (u64)97862);
    EXPECT_EQ(board.Perft(4), (u64)4085603);
    // EXPECT_EQ(board.Perft(5), (u64)193690690);
}
TEST(Perft, Position3) {
    Board board;
    board.SetFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
    EXPECT_EQ(board.Perft(1), (u64)14);
    EXPECT_EQ(board.Perft(2), (u64)191);
    EXPECT_EQ(board.Perft(3), (u64)2812);
    EXPECT_EQ(board.Perft(4), (u64)43238);
    EXPECT_EQ(board.Perft(5), (u64)674624);
}
TEST(Perft, Position4) {
    Board board;
    board.SetFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(board.Perft(1), (u64)6);
    EXPECT_EQ(board.Perft(2), (u64)264);
    EXPECT_EQ(board.Perft(3), (u64)9467);
    EXPECT_EQ(board.Perft(4), (u64)422333);
    // EXPECT_EQ(board.Perft(5), (u64)15833292);
}
TEST(Perft, Position5) {
    Board board;
    board.SetFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(board.Perft(1), (u64)44);
    EXPECT_EQ(board.Perft(2), (u64)1486);
    EXPECT_EQ(board.Perft(3), (u64)62379);
    EXPECT_EQ(board.Perft(4), (u64)2103487);
    // EXPECT_EQ(board.Perft(5), (u64)89941194);
}
TEST(Perft, Position6) {
    Board board;
    board.SetFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(board.Perft(1), (u64)46);
    EXPECT_EQ(board.Perft(2), (u64)2079);
    EXPECT_EQ(board.Perft(3), (u64)89890);
    EXPECT_EQ(board.Perft(4), (u64)3894594);
    // EXPECT_EQ(board.Perft(5), (u64)164075551);
}

// http://www.rocechess.ch/perft.html
TEST(Perft, RocePromotion) {
    Board board;
    board.SetFen("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    EXPECT_EQ(board.Perft(1), (u64)24);
    EXPECT_EQ(board.Perft(2), (u64)496);
    EXPECT_EQ(board.Perft(3), (u64)9483);
    EXPECT_EQ(board.Perft(4), (u64)182838);
    EXPECT_EQ(board.Perft(5), (u64)3605103);
}

// https://sites.google.com/site/numptychess/perft/
TEST(Perft, Numpty3) {
    Board board;
    board.SetFen("r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R w KQkq -");
    EXPECT_EQ(board.Perft(1), (u64)17);
    EXPECT_EQ(board.Perft(2), (u64)341);
    EXPECT_EQ(board.Perft(3), (u64)6666);
    EXPECT_EQ(board.Perft(4), (u64)150072);
    EXPECT_EQ(board.Perft(5), (u64)3186478);
}
TEST(Perft, Numpty4) { //enpassant check
    Board board;
    board.SetFen("8/5p2/8/2k3P1/p3K3/8/1P6/8 b - -");
    EXPECT_EQ(board.Perft(1), (u64)9);
    EXPECT_EQ(board.Perft(2), (u64)85);
    EXPECT_EQ(board.Perft(3), (u64)795);
    EXPECT_EQ(board.Perft(4), (u64)7658);
    // EXPECT_EQ(board.Perft(5), (u64)72170); //CHECK IF WRONG! 72120
    EXPECT_EQ(board.Perft(6), (u64)703851);
}
TEST(Perft, Numpty5) {
    Board board;
    board.SetFen("r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R b KQkq -");
    EXPECT_EQ(board.Perft(1), (u64)29);
    EXPECT_EQ(board.Perft(2), (u64)953);
    EXPECT_EQ(board.Perft(3), (u64)27990);
    EXPECT_EQ(board.Perft(4), (u64)909807);
}
TEST(Perft, PeterEllis) {
    Board board;
    board.SetFen("r6r/1b2k1bq/8/8/7B/8/8/R3K2R b QK - 3 2");
    EXPECT_EQ(board.Perft(1), (u64)8);

    board.SetFen("8/8/8/2k5/2pP4/8/B7/4K3 b - d3 5 3");
    EXPECT_EQ(board.Perft(1), (u64)8);

    board.SetFen("r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b QqKk - 3 2");
    EXPECT_EQ(board.Perft(1), (u64)5);

    board.SetFen("2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b QK - 3 2");
    EXPECT_EQ(board.Perft(1), (u64)44);

    board.SetFen("rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w QK - 3 9");
    EXPECT_EQ(board.Perft(1), (u64)39);

    board.SetFen("2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4");
    EXPECT_EQ(board.Perft(1), (u64)9);

    board.SetFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(board.Perft(3), (u64)62379);

    board.SetFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(board.Perft(3), (u64)89890);
    
    board.SetFen("3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1");
    EXPECT_EQ(board.Perft(6), (u64)1134888);

    board.SetFen("K1k5/8/P7/8/8/8/8/8 w - - 0 1");
    EXPECT_EQ(board.Perft(6), (u64)2217);
}
TEST(Perft, Talkchess) {
    //http://www.talkchess.com/forum3/viewtopic.php?t=59781
    Board board;
    board.SetFen("rnb1kbnr/pp1pp1pp/1qp2p2/8/Q1P5/N7/PP1PPPPP/1RB1KBNR b Kkq - 2 4");
    EXPECT_EQ(board.Perft(1), (u64)28);
    EXPECT_EQ(board.Perft(2), (u64)741);
    EXPECT_EQ(board.Perft(3), (u64)21395);
    EXPECT_EQ(board.Perft(4), (u64)583456);

    board.SetFen("1N6/6k1/8/8/7B/8/8/4K3 w - - 19 103");
    EXPECT_EQ(board.Perft(1), (u64)14);

    //http://talkchess.com/forum3/viewtopic.php?f=7&t=71379
    board.SetFen("8/ppp3p1/8/8/3p4/5Q2/1ppp2K1/brk4n w - - 11 7");
    EXPECT_EQ(board.Perft(1), (u64)27);
    EXPECT_EQ(board.Perft(2), (u64)390);
    EXPECT_EQ(board.Perft(3), (u64)9354);
    EXPECT_EQ(board.Perft(4), (u64)134167);

    board.SetFen("8/6kR/8/8/8/bq6/1rqqqqqq/K1nqnbrq b - - 0 1");
    EXPECT_EQ(board.Perft(1), (u64)7);
    EXPECT_EQ(board.Perft(2), (u64)52);
    EXPECT_EQ(board.Perft(3), (u64)4593);
    EXPECT_EQ(board.Perft(4), (u64)50268);
}
