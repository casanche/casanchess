#include "Uci.h"

#include "Board.h"
#include "Evaluation.h"
#include "Hash.h"
#include "NNUE.h"

#include <iostream>
#include <string>
#include <thread>

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
            std::cout << "option name NNUE_File type string" << std::endl;

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
        else if(token == "NNUE_File") {
            stream >> token;
            if(token != "value")
                return;
            stream >> token;
            P(token);

            nnue.Load(token.c_str());
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
