#include "Uci.h"

#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "NNUE.h"
#include "SearchLimits.h"
#include "Syzygy.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

////https://ucichessengine.wordpress.com/2011/03/16/description-of-uci-protocol/

namespace {
    const std::string ENGINE_NAME = "Casanchess";
    const std::string AUTHOR = "Carlos Sanchez Mayordomo";
    const std::string VERSION_MAJOR = "1";
    const std::string VERSION_MINOR = "1";
    const std::string VERSION_PATCH = "1";
}

Uci::Uci() :
    m_board(Board()),
    m_search(Search())
{}

Uci::~Uci() {
    StopAndJoin();
}

void Uci::Launch() {

    std::string line;

    while(std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string token;
        stream >> std::skipws >> token;

        if(token == "uci") {
            std::cout << "id name " << ENGINE_NAME << " " << VERSION_MAJOR << "." << VERSION_MINOR;
            if(VERSION_PATCH != "0")
                std::cout << "." << VERSION_PATCH;
            std::cout << std::endl;
            std::cout << "id author " << AUTHOR << std::endl;

            //Options
            std::cout << "option name ClassicalEval type check default false" << std::endl;
            std::cout << "option name ClearHash type button" << std::endl;
            std::cout << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min 1 max 4096" << std::endl;
            std::cout << "option name NNUE_Path type string default " << nnue.GetPath() << std::endl;
            std::cout << "option name Ponder type check default false" << std::endl;
            std::cout << "option name SyzygyPath type string default " << Syzygy::DEFAULT_PATH << std::endl;
            std::cout << "option name SyzygyProbeLimit type spin default " << UCI_SYZYGY_PROBE_LIMIT << " min 0 max 7" << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;

            std::cout << "uciok" << std::endl;
        }
        else if(token == "isready") { std::cout << "readyok" << std::endl; }
        else if(token == "setoption") { SetOption(stream); }
        else if(token == "ucinewgame") {
            StopAndJoin();

            m_search.ClearSearch(true);
            m_board.Init();
        }
        else if(token == "position") { Position(stream); }
        else if(token == "go") { Go(stream); }
        else if(token == "stop") { m_search.Stop(); }
        else if(token == "ponderhit") { m_search.PonderHit(); }
        else if(token == "quit" || token == "q") {
            m_search.Stop();
            std::_Exit(EXIT_SUCCESS);
        }
        //Non-UCI commands
        else if(token == "perft" || token == "divide") {
            int depth;
            stream >> depth;

            Utils::Clock clock;
            clock.Start();

            Board board;
            board.Print();
            if(token == "perft") {
                std::cout << board.Perft(depth) << std::endl;
            } else {
                board.Divide(depth);
            }

            std::cout << "[" << token << " " << depth << "] " << clock.Elapsed() << " ms" << std::endl;
        }
        else if(token == "bench") {
            int depth = 9; // Default depth
            if(stream >> depth) {
                std::cout << "Depth provided by user" << std::endl;
            }
            Bench(depth, true);
        }
        else if(token == "hashmoves") {
            m_board.ShowHashMoves();
        }
        else if(token == "mirror") {
            m_board.Mirror();
        }
        else if(token == "print") {
            m_board.Print();
            std::string activePlayer = (m_board.ActivePlayer() == WHITE) ? "WHITE" : "BLACK";
            P("Active player: " << activePlayer);
            P("Ply: " << m_board.Ply());
            P("Fifty-move rule: " << m_board.FiftyRule());
            std::string castlingRights;
            if(m_board.CastlingRights() & 0b0001) castlingRights += "K"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b0010) castlingRights += "Q"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b0100) castlingRights += "k"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b1000) castlingRights += "q"; else castlingRights += "-";
            P("Castling rights: " << castlingRights);
            Bitboard enpassant = m_board.EnPassantSquare();
            int epSquare = enpassant ? BitscanForward(enpassant) : -1;
            P("Enpassant square: " << epSquare);
            P("ZKey: " << m_board.ZKey());
            P("Static evaluation: " << Evaluation::Evaluate(m_board));
            std::cout << "Move history: "; m_board.ShowHistory(); std::cout << std::endl;
        }
        else {
            std::cout << "Command not valid: " << line << std::endl;
        }
    }
}

void Uci::Bench(int depth, bool verbose) {
    StopAndJoin();

    if(verbose)
        std::cout << "Running benchmark at depth " << depth << "..." << std::endl;

    // Varied test positions for benchmarking
    std::vector<std::string> testPositions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Starting position
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", // Perft 5
        "r1bqr1k1/ppp2ppp/3p1n2/4n3/2PNP3/2P2P2/P3B1PP/1RBQ1RK1 b - - 1 11", // RG16 - Opening
        "6r1/1ppnr1kp/pq1b1p2/2p1pPP1/P3P3/1P1P1N2/R1PB3Q/5R1K b - - 2 30", // RG10 - Midgame Closed
        "8/5p2/2B2k2/p7/1pr5/4r3/P1P3P1/3R2K1 w - - 12 48", // RG4 - Ending
        "r3qb1k/1b4p1/p2pr2p/3n4/Pnp1N1N1/6RP/1B3PP1/1B1QR1K1 w - - 0 1", // Nolot 1 - Wide tree
        "rnbq1b1r/p1pp1p1p/4k3/1p1NP1p1/2QP1p2/5N2/PP1B1KPP/n6R w - - 0 1", // Albillo 11 - Checks and mates
        "8/3P4/n2K2kp/2p3nN/1b6/2p1p1P1/8/3B4 w - - 4 3",  // Quiescence and deep pruning
        "8/4RNpk/p7/r1pr4/5P2/2P3K1/b2R4/8 w - - 1 40", // Lichess study - Search extensions 1
        "8/p7/2k1p3/5p1p/2KP1P1P/P7/8/8 b - - 0 37", // Lichess study - Pawn Endgame 1
    };

    u64 totalNodes = 0;
    int64_t totalTime = 0;
    int totalNps = 0;
    int positionCount = 0;

    // Disable UCI output during benchmark
    bool original_UciOutput = UCI_OUTPUT;
    UCI_OUTPUT = false;

    // Disable Syzygy tables
    bool original_SyzygyProbeLimit = UCI_SYZYGY_PROBE_LIMIT;
    UCI_SYZYGY_PROBE_LIMIT = 0;

    for(size_t i = 0; i < testPositions.size(); i++) {
        if (verbose) {
            std::cout << "Position " << (i + 1) << "/" << testPositions.size() << ":" << std::endl;
            std::cout << testPositions[i] << std::endl;
        }

        // Set position
        m_board.SetFen(testPositions[i]);

        // Set search limits
        m_search.IterativeDeepening(m_board, UCI_Limits::FixDepth(depth), true);

        // Get results using the engine's own calculations
        u64 nodes = m_search.GetNodes();
        i64 elapsed = m_search.GetLimits().ElapsedTime();
        int nps = m_search.GetLimits().CalculateNPS(nodes);

        totalNodes += nodes;
        totalTime += elapsed;
        totalNps += nps;
        positionCount++;

        if (verbose) {
            std::cout << "  Nodes: " << nodes << std::endl;
            std::cout << "  Time: " << elapsed << " ms" << std::endl;
            std::cout << "  Speed: " << std::fixed << std::setprecision(2) << (nps / 1000.0) << " kN/s" << std::endl;
            std::cout << "  Best move: " << m_search.BestMove().Notation() << std::endl;
            std::cout << "  Score: " << m_search.BestScore() << std::endl;
            std::cout << std::endl;
        }
    }

    // Restore UCI parameters
    UCI_OUTPUT = original_UciOutput;
    UCI_SYZYGY_PROBE_LIMIT = original_SyzygyProbeLimit;

    // Overall results
    double avgNps = positionCount > 0 ? (totalNps / (double)positionCount) : 0;
    if(verbose) {
        std::cout << "=== BENCHMARK RESULTS ===" << std::endl;
        std::cout << "Total nodes: " << totalNodes << std::endl;
        std::cout << "Total time: " << totalTime << " ms" << std::endl;
        std::cout << "Average speed: " << std::fixed << std::setprecision(2) << (avgNps / 1000.0) << " kN/s" << std::endl;
        std::cout << "========================" << std::endl;
    } else {
        std::cout << totalNodes << " nodes " << static_cast<int>(avgNps) << " nps" << std::endl;
    }
}

void Uci::Go(std::istringstream &stream) {
    StopAndJoin();
    
    std::string token;
    UCI_Limits limits;

    while(stream >> token) {

        if(token == "ponder") { limits.ponder = true; }
        else if(token == "infinite") limits.infinite = true;
        else if(token == "depth") stream >> limits.depth;
        else if(token == "movetime") stream >> limits.moveTime;
        else if(token == "nodes") stream >> limits.nodes;
        else if(token == "wtime") stream >> limits.wtime;
        else if(token == "btime") stream >> limits.btime;
        else if(token == "winc") stream >> limits.winc;
        else if(token == "binc") stream >> limits.binc;
        else if(token == "movestogo") stream >> limits.movesToGo;

        else {
            const uint DEFAULT_NODES = 150000;
            P("[WARNING] UNDEFINED GO STATEMENT: " << token << " -> Using default statement: 'go nodes " << DEFAULT_NODES << "'");
            limits.nodes = DEFAULT_NODES;
            break;
        }

    }

    m_limits = limits; // To avoid copies in std::thread that may cause memory misalignments
    m_searchThread = std::thread(&Uci::StartSearch, this);
}

void Uci::Position(std::istringstream &stream) {
    StopAndJoin();

    std::string token;

    while(stream >> token) {

        if(token == "startpos") {
            m_board.Init();
        }

        else if(token == "fen") {
            std::string fen;
            while(stream >> token && token != "moves") {
                fen += token + " ";
            }
            m_board.SetFen(fen);
        }

        while(stream >> token) {
            if(token == "moves") continue;
            // P(token);
            m_board.MakeMove(token);
        }

        // m_board.Print();
    }

}

void Uci::SetOption(std::istringstream &stream) {
    StopAndJoin();

    std::string token;
    stream >> token; //should be 'name'
    if(token != "name")
        return;

    while(stream >> token) {
        if(token == "Hash") {
            stream >> token; //should be 'value'
            if(token != "value")
                return;
            stream >> token;
            P(token);

            Hash::tt.SetSize( stoi(token) );
        }
        else if(token == "Ponder") {
            stream >> token; //should be 'value'
            if(token != "value")
                return;
            stream >> token;
            P(token);

            if(token == "true")
                UCI_PONDER = true;
            else if(token == "false")
                UCI_PONDER = false;
        }
        else if(token == "ClearHash") {
            Hash::tt.Clear();
        }
        else if(token == "ClassicalEval") {
            stream >> token;
            if(token != "value")
                return;
            stream >> token;
            P(token);

            if(token == "true")
                UCI_CLASSICAL_EVAL = true;
            else if(token == "false")
                UCI_CLASSICAL_EVAL = false;
        }
        else if(token == "NNUE_Path") {
            stream >> token;
            if(token != "value")
                return;

            std::string path;

            stream >> std::ws; // Skip leading whitespaces
            std::getline(stream, path); // Support spaces

            if(path.ends_with("\r"))
                path.pop_back();

            nnue.Load(path);
        }
        else if (token == "SyzygyPath") {
            stream >> token;
            if(token != "value")
                return;
            stream >> token;

            std::string path = token;
            Syzygy::Init(path);
        }
        else if (token == "SyzygyProbeLimit") {
            stream >> token;
            if(token != "value")
                return;
            stream >> token;

            int limit = std::stoi(token);
            UCI_SYZYGY_PROBE_LIMIT = limit;
            std::cout << "info string SyzygyProbeLimit set to: " << limit << std::endl;
        }
        else {
            std::cout << "Unknown option: " << token << std::endl;
            return;
        }
    }
}

void Uci::StartSearch() {
    m_search.IterativeDeepening(m_board, m_limits);
}

void Uci::StopAndJoin() {
    m_search.Stop();

    // Thread join
    if(m_searchThread.joinable()) {
        m_searchThread.join();
    }
}

// =================
// == UCI Outputs ==
// =================

void Uci::Output(int depth, int seldepth, int score, u64 nodes, i64 time, uint nps, int tbHits, BOUND_TYPE bound, const std::string& PV) {
    if(!UCI_OUTPUT) return;

    std::cout << "info depth " << depth;
    std::cout << " seldepth " << seldepth;

    if(IsMateValue(score)) {
        int mateScore = (score > 0) ?  MATESCORE_MAX - score + 1
                                    : -MATESCORE_MAX - score - 1;
        std::cout << " score mate " << mateScore / 2; //return mate in moves, not in plies
    } else {
        std::cout << " score cp " << score;
    }
    if(bound == BOUND_TYPE::LOWER_BOUND) std::cout << " lowerbound";
    if(bound == BOUND_TYPE::UPPER_BOUND) std::cout << " upperbound";

    std::cout << " time " << time;
    std::cout << " nodes " << nodes;
    std::cout << " nps " << nps;

    if(time > 1000)
        std::cout << " hashfull " << Hash::tt.Occupancy();
    if(tbHits)
        std::cout << " tbhits " << tbHits;

    std::cout << " pv " << PV;
    std::cout << std::endl;
}

void Uci::RootUpdate(int depth, int seldepth, int currMoveNumber, const std::string& currMove, u64 nodes, i64 time, uint nps, int tbHits) {
    if(!UCI_OUTPUT) return;
    if(time < UCI_OUTPUT_ROOTMAX_MINTIME) return;
    
    std::cout << "info depth " << depth
              << " seldepth " << seldepth
              << " currmovenumber " << currMoveNumber
              << " currmove " << currMove
              << " time " << time
              << " nodes " << nodes
              << " nps " << nps;
    if(tbHits)
        std::cout << " tbhits " << tbHits;
    std::cout << std::endl;
}

void Uci::BestMove(const std::string& bestmove, const std::string& pondermove) {
    if(!UCI_OUTPUT) return;

    std::cout << "bestmove " << bestmove;
    if(UCI_PONDER && !pondermove.empty())
        std::cout << " ponder " << pondermove;
    std::cout << std::endl;
}
