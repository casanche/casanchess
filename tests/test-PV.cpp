#include "Board.h"
#include "MoveGenerator.h"
#include "Search.h"
#include "Uci.h"

#include "test-Common.h"
using namespace TestCommon;

#include <sstream>
#include <string>
#include <vector>

class PVTest : public ::testing::Test {
protected:
    TT tt;
    Search search{tt};
    Board board;

    struct SearchOutput {
        std::vector<std::string> lines; // full UCI output
        std::vector<std::string> lastPV;
        std::string bestmove;
        std::string ponder;
    };

    void SetUp() override {
        UCI_PONDER = true;
    }

    void TearDown() override {
        UCI_PONDER = false;
    }

    SearchOutput RunSearch(const UCI_Limits& limits, bool fullClear = true) {
        SearchOutput result;

        // Capture the UCI output
        CaptureCout capture;
        search.IterativeDeepening(board, limits, fullClear);
        std::istringstream output(capture.Str());

        for(std::string line; std::getline(output, line); ) {
            // Whole line
            result.lines.push_back(line);

            // Last PV line
            const auto pv = InfoPV(line);
            if(!pv.empty())
                result.lastPV = pv;

            // Bestmove and ponder
            if(line.starts_with("bestmove ")) {
                result.bestmove = FieldValue(line, "bestmove");
                result.ponder = FieldValue(line, "ponder");
            }
        }

        return result;
    }

    // For a given UCI line, get the value for a field (e.g. get the number of nodes)
    static std::string FieldValue(const std::string& line, const std::string& name) {
        std::istringstream tokens(line);
        for(std::string token; tokens >> token; ) {
            if(token == name && tokens >> token)
                return token;
        }
        return "";
    }

    // For a given UCI 'info' line, get the PV moves
    static std::vector<std::string> InfoPV(const std::string& line) {
        std::vector<std::string> pv;

        const std::string pvToken = " pv ";
        const auto pvStart = line.find(pvToken);
        if(pvStart == std::string::npos)
            return pv;

        std::istringstream moves(line.substr(pvStart + pvToken.size()));
        for(std::string move; moves >> move; )
            pv.push_back(move);

        return pv;
    }

    static bool IsLegalMove(Board& board, const std::string& notation) {
        const MoveList legalMoves = MoveGenerator::GenerateMoves(board);
        for(const auto move : legalMoves) {
            if(move.Notation() == notation) {
                return true;
            }
        }
        return false;
    }

    static bool IsLegalLine(Board& board, const std::vector<std::string>& notations) {
        size_t plies = 0;

        for(const std::string& notation : notations) {
            if(!IsLegalMove(board, notation))
                break;

            board.MakeMove(notation);
            plies++;
        }

        const bool legal = plies == notations.size();

        for(size_t i = 0; i < plies; i++)
            board.TakeMove();

        return legal;
    }
};

// ==============
// == Legality ==
// ==============

TEST_F(PVTest, Legality) {
    board.SetFen("r3k2r/1P6/8/3Pp3/8/8/6p1/R3K2R w KQkq e6 0 1"); // position with special moves (castling, enpassant, promotions)

    for(const int nodes : {100, 1000, 5000}) {
        tt.Clear();
        const SearchOutput output = RunSearch(UCI_Limits::FixNodes(nodes));

        for(const std::string& line : output.lines) {
            const auto pv = InfoPV(line);
            EXPECT_TRUE(IsLegalLine(board, pv)) << "Illegal PV: " << line;
        }

        ASSERT_FALSE(output.lastPV.empty());
        EXPECT_EQ(output.bestmove, output.lastPV[0]);

        if(!output.ponder.empty()) {
            EXPECT_TRUE(IsLegalLine(board, {output.bestmove, output.ponder})) << "Illegal ponder: " << output.ponder;
        }
    }
}

// ============
// == Mating ==
// ============

TEST_F(PVTest, Mate) {
    board.SetFen("8/8/8/8/4K1Q1/8/7k/8 w - - 0 1");
    const SearchOutput result = RunSearch(UCI_Limits::FixDepth(3));

    EXPECT_EQ(search.BestScore(), MateAtPly(3));
    EXPECT_EQ(result.lastPV.size(), 3);
}

// ===================
// == Interruptions ==
// ===================

TEST_F(PVTest, Stopped_NewRoot) {
    const SearchOutput previous = RunSearch(UCI_Limits::FixDepth(2));

    EXPECT_FALSE(previous.ponder.empty());

    board.SetFen("4k3/8/8/8/8/8/8/4K2R w K - 0 1"); // startpos moves are not legal here
    ASSERT_FALSE(IsLegalMove(board, previous.bestmove));

    const SearchOutput current = RunSearch(UCI_Limits::FixNodes(1), false);

    EXPECT_TRUE(IsLegalMove(board, current.bestmove));
    EXPECT_TRUE(current.ponder.empty());
}

// Interrupt the search at next depth before the first rootmove is completed
TEST_F(PVTest, Stopped_NextDepth) {
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(2));
    const int nodes = static_cast<int>(search.GetNodes());

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(nodes + 1));

    ASSERT_FALSE(completed.ponder.empty());
    EXPECT_EQ(completed.bestmove, stopped.bestmove);
    EXPECT_EQ(completed.ponder, stopped.ponder);

    // The stopped iteration should not print a PV
    for(const std::string& line : stopped.lines) {
        if(!InfoPV(line).empty()) {
            EXPECT_NE(FieldValue(line, "depth"), "3");
        }
    }
}

// Fail-high in the root + new move
// Bestmove changes and ponder is empty
TEST_F(PVTest, Stopped_FailHigh_NewMove) {
    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(4));

    int failHighNodes = 0;
    std::string failHighMove;
    std::vector<std::string> previousPV;
    for(const std::string& line : completed.lines) {
        const auto pv = InfoPV(line);
        if(pv.empty()) continue;

        const bool failHigh = line.contains(" lowerbound ") && pv.size() == 1;
        if(failHigh && previousPV.size() > 1 && pv[0] != previousPV[0]) {
            failHighMove = pv[0];
            failHighNodes = std::stoi( FieldValue(line, "nodes") );
            break;
        }
        previousPV = pv;
    }

    if(failHighMove.empty())
        GTEST_SKIP() << "This test needs fail-high at root with a new move";

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(failHighNodes + 1));

    EXPECT_EQ(stopped.bestmove, failHighMove);
    EXPECT_TRUE(stopped.ponder.empty());
}

// Fail-high in the root + same move
// Bestmove remains the same and ponder is available
TEST_F(PVTest, Stopped_FailHigh_SameMove) {
    board.SetFen("3r2k1/7p/p7/3r1R2/PQ2K1P1/3p2PP/1P2b3/8 b - - 1 35");
    const SearchOutput completed = RunSearch(UCI_Limits::FixDepth(6));

    int failHighNodes = 0;
    std::string failHighMove;
    std::string expectedPonder;
    std::vector<std::string> previousPV;
    for(const std::string& line : completed.lines) {
        const auto pv = InfoPV(line);
        if(pv.empty()) continue;

        const bool failHigh = line.contains(" lowerbound ") && pv.size() == 1;
        if(failHigh && previousPV.size() > 1 && pv[0] == previousPV[0]) {
            failHighMove = pv[0];
            failHighNodes = std::stoi( FieldValue(line, "nodes") );
            expectedPonder = previousPV[1];
            break;
        }
        previousPV = pv;
    }

    if(failHighMove.empty())
        GTEST_SKIP() << "This test needs fail-high at root with the same move";

    tt.Clear();
    const SearchOutput stopped = RunSearch(UCI_Limits::FixNodes(failHighNodes + 1));

    EXPECT_EQ(stopped.bestmove, failHighMove);
    EXPECT_EQ(stopped.ponder, expectedPonder);
}
