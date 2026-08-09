#include "Interface.h"

#include "MoveGenerator.h"
#include "Search.h"

#include <iostream>

const std::string prefix = ">";

Interface::Interface() {
    NewGame();
}

void Interface::NewGame() {
    m_board.Init();
}

void Interface::Print() {
    m_board.Print();
}

void Interface::Start(std::string fenString) {
    PrintWelcome();

    if(fenString == "") {
        NewGame();
    } else {
        m_board.SetFen(fenString);
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
            m_board.SetFen(input);
        }

        //Random move
        else if(input == "random") {
            MoveList moves = MoveGenerator::GenerateMoves(m_board);

            if( !moves.empty() ) {
                m_board.MakeMove( MoveGenerator::RandomMove(moves) );
            } else {
                if( m_board.IsCheck() ) {
                    P("Checkmate!!!")
                } else {
                    P("Stalemate...")
                }
            }
            
        }

        //Think and make a move
        else if(input == "think" || input == "t") {
            Search search;

            search.IterativeDeepening(m_board, UCI_Limits::FixDepth(11));
            search.MakeMove(m_board);
        }

        //Show the list of moves
        else if(input == "moves") {
            m_board.ShowMoves();
        }

        //Divide-perft
        else if(input.find("divide") != std::string::npos) {
            input.erase(0, 7);
            m_board.Divide( stoi(input) );
        }

        else {
            m_board.MakeMove(input);
        }

    }
}

void Interface::PrintWelcome() {
    P(" CASANCHESS ");
    P(" Author: Carlos Sánchez Mayordomo ");
}
