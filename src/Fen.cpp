#include "Fen.h"
#include "Board.h"

#include <cassert>
#include <sstream>

void Fen::SetPosition(Board& board, std::string fenString) {
    board.ClearBits();

    std::istringstream fenStream(fenString);
    std::string token;

    int boardPos = A8; //starts at square '56'
    const int nextRank = -8; //go down one rank

    //Set the board pieces
    fenStream >> token;
    for(auto theChar : token) {
        switch(theChar) {
            case 'P': board.m_pieces[WHITE][PAWN]   |= SquareBB(boardPos++); break;
            case 'N': board.m_pieces[WHITE][KNIGHT] |= SquareBB(boardPos++); break;
            case 'B': board.m_pieces[WHITE][BISHOP] |= SquareBB(boardPos++); break;
            case 'R': board.m_pieces[WHITE][ROOK]   |= SquareBB(boardPos++); break;
            case 'Q': board.m_pieces[WHITE][QUEEN]  |= SquareBB(boardPos++); break;
            case 'K': board.m_pieces[WHITE][KING]   |= SquareBB(boardPos++); break;
            case 'p': board.m_pieces[BLACK][PAWN]   |= SquareBB(boardPos++); break;
            case 'n': board.m_pieces[BLACK][KNIGHT] |= SquareBB(boardPos++); break;
            case 'b': board.m_pieces[BLACK][BISHOP] |= SquareBB(boardPos++); break;
            case 'r': board.m_pieces[BLACK][ROOK]   |= SquareBB(boardPos++); break;
            case 'q': board.m_pieces[BLACK][QUEEN]  |= SquareBB(boardPos++); break;
            case 'k': board.m_pieces[BLACK][KING]   |= SquareBB(boardPos++); break;
            case '/': boardPos += nextRank * 2; break;
            default: boardPos += (int)(theChar - '0'); //numbers '1' to '9'
        };
    }

    //Active player
    fenStream >> token;
    board.m_activePlayer = (token == "w") ? WHITE : BLACK;

    //Castling
    fenStream >> token;
    if(token != "-") {
        for(auto theChar : token) {
            switch(theChar) {
                case 'K': board.m_castlingRights += CASTLING_K; break;
                case 'Q': board.m_castlingRights += CASTLING_Q; break;
                case 'k': board.m_castlingRights += CASTLING_k; break;
                case 'q': board.m_castlingRights += CASTLING_q; break;
                default: break;
            };
        }
    }

    //En passant
    fenStream >> token;
    if(token != "-") {
        int square = board.SquareToIndex(token);
        board.m_enPassantSquare = SquareBB(square);
    }

    //50-move rule
    fenStream >> token;
    if(token != "-")
        board.m_fiftyrule = std::stoi(token);

    //Move number
    fenStream >> token;
    board.m_moveNumber = 1, board.m_ply = 0, board.m_initialPly = 0;
    if(token != "-" && token != "0") {
        board.m_moveNumber = std::stoi(token);
        board.m_ply = (board.m_moveNumber-1) * 2;
        board.m_ply += board.m_activePlayer; //add one if white already moved
        board.m_initialPly = board.m_ply;
    }

    //Update state and history
    board.UpdateBitboards();
    assert(board.m_ply >= 0);
    board.m_history[board.m_ply].fiftyrule = board.m_fiftyrule;
    board.m_history[board.m_ply].castling = board.m_castlingRights;
    board.m_history[board.m_ply].enpassant = board.m_enPassantSquare;
    board.m_zobristKey.SetKey(board);
    board.m_history[board.m_ply].zkey = board.ZKey();
    board.m_checkCalculated = false;
}
