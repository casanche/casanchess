#include "Interface.h"

#include "Engine.h"
#include "MoveGenerator.h"

#include <iostream>

const int fixedNodes = 250000;
const std::string prefix = ">";

Interface::Interface() : m_engine(std::make_unique<Engine>()) {
    m_engine->NewGame();
}

Interface::~Interface() = default;

void Interface::Print() {
    m_engine->board.Print();
}

void Interface::Start(std::string fenString) {
    Board& board = m_engine->board;
    Search& search = m_engine->search;

    PrintWelcome();

    if(fenString == "") {
        m_engine->NewGame();
    } else {
        board.SetFen(fenString);
    }

    std::string input;

    while(true) {
        Print();
        std::cout << prefix;
        // std::cin >> input;
        std::getline(std::cin, input);

        if(input == "exit" || input == "e" || input == "quit" || input == "q") {
            break;
        }

        //Set the board position from a fen string
        else if(input.find("fen") != std::string::npos) {
            input.erase(0, 4); //pos 0, size 4
            board.SetFen(input);
        }

        //Random move
        else if(input == "random") {
            MoveList moves = MoveGenerator::GenerateMoves(board);

            if( !moves.empty() ) {
                board.MakeMove( MoveGenerator::RandomMove(moves) );
            } else {
                if( board.IsCheck() ) {
                    P("Checkmate!!!")
                } else {
                    P("Stalemate...")
                }
            }
            
        }

        //Think and make a move
        else if(input == "think" || input == "t") {
            search.IterativeDeepening(board, UCI_Limits::FixNodes(fixedNodes));
            search.MakeMove(board);
        }

        //Show the list of moves
        else if(input == "moves") {
            board.ShowMoves();
        }

        //Divide-perft
        else if(input.find("divide") != std::string::npos) {
            input.erase(0, 7);
            board.Divide( stoi(input) );
        }

        else {
            board.MakeMove(input);
        }

    }
}

void Interface::PrintWelcome() {
    P(" CASANCHESS ");
    P(" Author: Carlos Sanchez Mayordomo ");
}
