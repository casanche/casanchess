#include "Uci.h"

#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "NNUE.h"

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

bool UCI_PONDER = false;
bool UCI_OUTPUT = true;
bool UCI_CLASSICAL_EVAL = false;

////https://ucichessengine.wordpress.com/2011/03/16/description-of-uci-protocol/

namespace {
    const std::string ENGINE_NAME = "Casanchess";
    const std::string AUTHOR = "Carlos Sanchez Mayordomo";
    const std::string VERSION_MAJOR = "0";
    const std::string VERSION_MINOR = "8";
    const std::string VERSION_PATCH = "0";
}

Uci::Uci() :
    m_board(Board()),
    m_search(Search())
{}

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
            std::cout << "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min 1 max 4096" << std::endl;
            std::cout << "option name Ponder type check default false" << std::endl;
            std::cout << "option name ClearHash type button" << std::endl;
            std::cout << "option name ClassicalEval type check default false" << std::endl;
            std::cout << "option name NNUE_Path type string default " << nnue.GetPath() << std::endl;

            std::cout << "uciok" << std::endl;
        }
        else if(token == "debug") {
            m_search.DebugMode();
        }
        else if(token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if(token == "setoption") {
            SetOption(stream);
        }
        else if(token == "ucinewgame") {
            m_board.Init();
        }
        else if(token == "position") {
            Position(stream);
        }
        else if(token == "go") {
            Go(stream);
        }
        else if(token == "stop") {
            m_search.Stop();
        }
        else if(token == "ponderhit") {
            Limits limits = m_search.GetLimits();
            limits.infinite = false;
            limits.ponderhit = true;
            m_search.AllocateLimits(m_board, limits);
        }
        else if(token == "quit" || token == "q") {
            std::cout << "info string quitting" << std::endl;
            break;
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
                P( board.Perft(depth) );
            } else {
                board.Divide(depth);
            }

            std::cout << "[" << token << " " << depth << "] " << clock.Elapsed() << " ms" << std::endl;
        }
        else if(token == "bench") {
            int depth = 8; // Default depth
            stream >> depth;
            
            std::cout << "Running benchmark at depth " << depth << "..." << std::endl;
            
            // Standard test positions for chess engine benchmarking
            std::vector<std::string> testPositions = {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Starting position
                "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
                "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", // Position 3
                "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", // Position 4
                "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", // Position 5
                "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", // Position 6
                "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1", // Sicilian
                "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 3 2", // Sicilian after Nf3
                "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 3", // Sicilian after Nf3 Nf6
                "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 5 3"  // Sicilian after Nf3 Nf6 e3
            };
            
            u64 totalNodes = 0;
            int totalTime = 0;
            int totalNps = 0;
            int positionCount = 0;
            
            // Disable UCI output during benchmark
            bool originalUciOutput = UCI_OUTPUT;
            UCI_OUTPUT = false;
            
            for(size_t i = 0; i < testPositions.size(); i++) {
                std::cout << "Position " << (i + 1) << "/" << testPositions.size() << ":" << std::endl;
                
                // Set position
                m_board.SetFen(testPositions[i]);
                
                // Set search limits
                Limits limits;
                limits.depth = depth;
                limits.infinite = false;
                
                // Clear hash table for fair comparison
                Hash::tt.Clear();
                Hash::pawnHash.Clear();
                
                // Run search
                m_search.AllocateLimits(m_board, limits);
                m_search.IterativeDeepening(m_board);
                
                // Get results using the engine's own calculations
                int elapsed = m_search.ElapsedTime();
                u64 nodes = m_search.GetNodes();
                int nps = m_search.GetNps();
                
                totalNodes += nodes;
                totalTime += elapsed;
                totalNps += nps;
                positionCount++;
                
                std::cout << "  Nodes: " << nodes << std::endl;
                std::cout << "  Time: " << elapsed << " ms" << std::endl;
                std::cout << "  Speed: " << std::fixed << std::setprecision(2) << (nps / 1000.0) << " kN/s" << std::endl;
                std::cout << "  Best move: " << m_search.BestMove().Notation() << std::endl;
                std::cout << "  Score: " << m_search.BestScore() << std::endl;
                std::cout << std::endl;
            }
            
            // Restore UCI output
            UCI_OUTPUT = originalUciOutput;
            
            // Overall results
            double avgNps = positionCount > 0 ? (totalNps / (double)positionCount) : 0;
            std::cout << "=== BENCHMARK RESULTS ===" << std::endl;
            std::cout << "Total nodes: " << totalNodes << std::endl;
            std::cout << "Total time: " << totalTime << " ms" << std::endl;
            std::cout << "Average speed: " << std::fixed << std::setprecision(2) << (avgNps / 1000.0) << " kN/s" << std::endl;
            std::cout << "========================" << std::endl;
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
            std::string castlingRights;
            if(m_board.CastlingRights() & 0b0001) castlingRights += "K"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b0010) castlingRights += "Q"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b0100) castlingRights += "k"; else castlingRights += "-";
            if(m_board.CastlingRights() & 0b1000) castlingRights += "q"; else castlingRights += "-";
            P("Active player: " << activePlayer);
            P("Ply: " << m_board.Ply());
            P("Fifty-move rule: " << m_board.FiftyRule());
            P("Castling rights: " << castlingRights);
            P("Enpassant square: " << BitscanForward(m_board.EnPassantSquare()));
            P("ZKey: " << m_board.ZKey());
            P("Static evaluation: " << Evaluation::Evaluate(m_board));
            std::cout << "Move history: "; m_board.ShowHistory(); std::cout << std::endl;
        }
        else {
            std::cout << "Command not valid: " << line << std::endl;
        }
    }

}

void Uci::Go(std::istringstream &stream) {
    std::string token;
    Limits limits;

    while(stream >> token) {

        if(token == "ponder") { limits.infinite = true; stream >> token; }
        if(token == "infinite") limits.infinite = true;
        else if(token == "depth") stream >> limits.depth;
        else if(token == "movetime") stream >> limits.moveTime;
        else if(token == "nodes") stream >> limits.nodes;
        else if(token == "wtime") stream >> limits.wtime;
        else if(token == "btime") stream >> limits.btime;
        else if(token == "winc") stream >> limits.winc;
        else if(token == "binc") stream >> limits.binc;
        else if(token == "movestogo") stream >> limits.movesToGo;

        else {
            std::string temp;
            stream >> temp;
            P("UNDEFINED GO STATMENT: " << temp << " -- (LOADING DEFAULT VALUES) -- ");
            m_search.FixTime(2000);
            break;
        }

    }

    m_search.AllocateLimits(m_board, limits);

    std::thread thread(&Uci::StartSearch, this);
    thread.detach();
}

void Uci::Position(std::istringstream &stream) {
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
            stream >> token;

            nnue.Load(token);
        }
        else {
            std::cout << "Unknown option: " << token << std::endl;
            return;
        }
    }
}

void Uci::StartSearch() {
    m_search.IterativeDeepening(m_board);
}
