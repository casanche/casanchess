#include "Board.h"

#include "test-Common.h"
using namespace TestCommon;


TEST(SEE, WinningCapture) {
    Board board;
    // Pawn takes undefended knight: Knight value (350)
    board.SetFen("4k3/8/8/3n4/4P3/8/8/4K3 w - -");
    Move move(E4, D5, PAWN, CAPTURE);
    move.SetCapturedType(KNIGHT);
    EXPECT_EQ(board.SEE(move), 350);
}

TEST(SEE, LosingCapture) {
    Board board;
    // Knight takes pawn defended by pawn: Pawn - Knight = -250
    board.SetFen("4k3/8/4p3/3p4/8/4N3/8/4K3 w - -");
    Move move(E3, D5, KNIGHT, CAPTURE);
    move.SetCapturedType(PAWN);
    EXPECT_EQ(board.SEE(move), -250);
}

TEST(SEE, EqualExchange) {
    Board board;
    // Knight takes knight, recaptured by knight: 0
    board.SetFen("4k3/2n5/8/3n4/8/4N3/8/4K3 w - -");
    Move move(E3, D5, KNIGHT, CAPTURE);
    move.SetCapturedType(KNIGHT);
    EXPECT_EQ(board.SEE(move), 0);
}

TEST(SEE, XRayRookBehindRook) {
    Board board;
    // RxP with rook x-ray behind: pawn value (100)
    board.SetFen("4k3/8/8/3p4/3R4/8/3R4/4K3 w - -");
    Move move(D4, D5, ROOK, CAPTURE);
    move.SetCapturedType(PAWN);
    EXPECT_EQ(board.SEE(move), 100);
}

TEST(SEE, QueenTakesDefendedPawn) {
    Board board;
    // QxP defended by pawn: Pawn - Queen = -950
    board.SetFen("4k3/8/2p5/3p4/4Q3/8/8/4K3 w - -");
    Move move(E4, D5, QUEEN, CAPTURE);
    move.SetCapturedType(PAWN);
    EXPECT_EQ(board.SEE(move), -950);
}

// Bishop and pawn aligned. Pawn captures knight, bishop x-ray behind pawn.
//       p
//     n
//   P
// B
TEST(SEE, XRayBlockedByPawn) {
    Board board;
    board.SetFen("3k4/8/4p3/3n4/2P5/1B6/8/3K4 w - - 0 1");
    
    Move move(C4, D5, PAWN, CAPTURE);
    move.SetCapturedType(KNIGHT);
    EXPECT_EQ(board.SEE(move), 350);
}

TEST(SEE, EnPassant) {
    Board board;
    board.SetFen("3n3k/8/8/3Pp3/8/8/8/K3R3 w - e6 0 1");
    
    Move move(D5, E6, PAWN, ENPASSANT);
    move.SetCapturedType(PAWN);
    EXPECT_EQ(board.SEE(move), 100);
}

TEST(SEE, PromotionRecapture_EarlyStop) {
    Board board;
    board.SetFen("3r4/2P2n2/4Nk2/8/8/8/8/K7 w - - 0 1");
    
    Move move(E6, D8, KNIGHT, CAPTURE);
    move.SetCapturedType(ROOK);

    // The SEE algorithm will see that after Nxd8 Nxd8, the pawn on c7 can recapture
    // and promote to a queen (unfavourable for black).
    // Therefore, black does not take the attacking piece.
    // (net gain for white: +rook).
    EXPECT_EQ(board.SEE(move), 500);
}

TEST(SEE, PromotionRecapture_FullSequence) {
    Board board;
    board.SetFen("Q2r4/2P2n2/5k2/b7/8/8/8/K7 w - - 0 1");
    
    Move move(A8, D8, QUEEN, CAPTURE);
    move.SetCapturedType(ROOK);

    // The SEE algorithm will see that after Qxd8 Nxd8, the pawn on c7 can recapture
    // and promote to a queen. But this time recapturing is favourable for black.
    // Therefore, black takes the attacking piece.
    // (net gain for white: +rook -queen +knight +(queen-pawn) -queen).
    EXPECT_EQ(board.SEE(move), -300);
}

// A white rook (defended by a queen) takes a black pawn (defended by the king).
// So the black king can't recapture the rook or it would be captured.
//
// The test verifies that the king has a large material value
// to stop the sequence early if the king is captured.
TEST(SEE, KingRecapture) {
    Board board;
    board.SetFen("8/4k3/4p3/8/4R3/8/4Q3/4K3 w - - 0 1");

    Move move(E4, E6, ROOK, CAPTURE);
    move.SetCapturedType(PAWN);

    EXPECT_EQ(board.SEE(move), 100);
}
