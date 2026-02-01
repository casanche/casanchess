#include "Fen.h"
#include "Board.h"

#include <cassert>
#include <sstream>

void Fen::SetPosition(Board& board, std::string fenString) {
    board.ClearBits();

    std::istringstream fenStream(fenString);
    std::string token;

    int boardPos = A8; //starts at square '56'
    const int nextRank = 8;

    //Set the board pieces
    fenStream >> token;
    for(char theChar : token) {
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
            case '/': boardPos -= nextRank * 2; break; //go down two ranks
            default: boardPos += (int)(theChar - '0'); //numbers '1' to '9'
        };
    }

    //Active player
    fenStream >> token;
    board.m_activePlayer = (token == "w") ? WHITE : BLACK;

    //Castling
    token = "-";
    fenStream >> token;
    if(token != "-") {
        for(char theChar : token) {
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
    token = "-";
    fenStream >> token;
    if(token != "-") {
        int square = board.SquareToIndex(token);
        board.m_enPassantSquare = SquareBB(square);
    }

    //50-move rule
    token = "-";
    fenStream >> token;
    if(token != "-")
        board.m_fiftyrule = SafeCastU8(std::stoi(token));

    //Move number
    token = "-";
    fenStream >> token;
    if(token != "-" && token != "0") {
        board.m_moveNumber = std::stoi(token);
        board.m_ply = (board.m_moveNumber-1) * 2;
        board.m_ply += board.m_activePlayer; //add one if white already moved
        board.m_initialPly = board.m_ply;
    }

    board.InitStateAndHistory();
}

//Fen with only the piece positions (without side-to-move, castling rights...)
std::string Fen::GetSimplifiedFen(const Board& board) {
    std::string buffer = ""; //buffer to fill the fen
    int empties = 0; //number of successive empty squares

    const char PIECES_NOTATION[2][8] = {{'\0', 'P', 'N', 'B', 'R', 'Q', 'K', '\0'},  //white
                                        {'\0', 'p', 'n', 'b', 'r', 'q', 'k', '\0'}}; //black

    for(int rank = RANK8; rank >= RANK1; rank--) {
        for(int file = FILEA; file <= FILEH; file++) {
            int square = rank*8 + file;

            char whitePiece = PIECES_NOTATION[WHITE][board.GetPieceAtSquare(WHITE, square)];
            char blackPiece = PIECES_NOTATION[BLACK][board.GetPieceAtSquare(BLACK, square)];

            //Lambda function. Writes the number of empties to the buffer and resets the counter
            auto bufferEmpties = [&] () {
                if(empties > 0)
                    buffer += std::to_string(empties);
                empties = 0;
            };

            if(whitePiece) {
                bufferEmpties();
                buffer += whitePiece;
            }
            else if(blackPiece) {
                bufferEmpties();
                buffer += blackPiece;
            }
            else {
                empties++;
            }

            if(file == FILEH) {
                bufferEmpties();
                if(rank != RANK1)
                    buffer += "/";
            }
        } //file
    } //rank

    return buffer;
}

EPDLine Fen::ReadEPDLine(const std::string& line) {
    std::string name, content, temp;
    std::istringstream stream(line);

    EPDLine epdline;

    //Fen
    name = "fen";
    content = "";
    stream >> content;
    stream >> temp; content += " " + temp;
    stream >> temp; content += " " + temp;
    stream >> temp; content += " " + temp;
    content += " -";
    epdline[name] = content;

    //Field
    while(stream >> temp) {
        name = temp;
        content = "";
        while(content.back() != ';') {
            stream >> temp; content += " " + temp;
        }
        content.erase(0, 1);
        content.pop_back();
        epdline[name] = content;
    }

    return epdline;
}