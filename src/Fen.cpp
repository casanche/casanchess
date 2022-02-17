#include "Fen.h"
#include "Board.h"
#include "Utils.h"

#include <cassert>
#include <sstream>

namespace {
    const Bitboard LIGHT_SQUARES = 0x55AA55AA55AA55AA;
    const Bitboard DARK_SQUARES = 0xAA55AA55AA55AA55;

    bool RedundantBishop(Board& board, COLOR c, Bitboard newBishop) {
        Bitboard oldBishop = board.Piece(c, BISHOP);
        return ((oldBishop & LIGHT_SQUARES) && (newBishop & LIGHT_SQUARES)) 
            || ((oldBishop & DARK_SQUARES) && (newBishop & DARK_SQUARES));
    }
}

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
        board.m_fiftyrule = std::stoi(token);

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

//Sets the board with a random fen string
//Returns the sfen
std::string Fen::SetRandomPosition(Board& board) {
    board.ClearBits();

    Utils::PRNG rng;

    const int pieceMax[8] = {0, 8, 2, 2, 2, 1, 1, 0};

    auto CheckIsValid = [&] (COLOR c, int p, Bitboard bb) {
        if(bb & board.m_allpieces)
            return false;
        int square = BitscanForward(bb);
        if(p == PAWN && (Rank(square) == RANK1 || Rank(square) == RANK8))
            return false;
        if(p == PAWN && RelativeRank(c, square) == RANK7)
            return false;
        if(p == BISHOP && RedundantBishop(board, c, bb))
            return false;
        int nHeavyPieces = PopCount(board.m_allpieces ^ board.Piece(c, PAWN) ^ board.Piece((COLOR)!c, PAWN));
        if(p == KING && nHeavyPieces >= 9 && nHeavyPieces <= 12 && RelativeRank(c, square) > RANK4)
            return false;
        if(p == KING && nHeavyPieces >= 13 && RelativeRank(c, square) > RANK2)
            return false;
        if(p != PAWN && board.AttackersTo(c, square))
            return false;
        return true;
    };

    do {
        board.ClearBits();

        for(int c = WHITE; c <= BLACK; c++) {
            for(int p = PAWN; p <= KING; p++) {

            const int max = (p == KING) ? 1 : rng.Random(0, pieceMax[p]);

            for(int m = 1; m <= max; m++) {
                Bitboard randomSquareBB;
                bool isValid;
                int tries = 0;
                do {
                    tries++;
                    int random = rng.Random(1,3);
                    int randomSquare;
                    if(random == 3)
                        randomSquare = (c == WHITE) ? rng.Random(32, 63) : rng.Random(0, 31);
                    else
                        randomSquare = (c == WHITE) ? rng.Random(0, 31) : rng.Random(32, 63);
                    randomSquareBB = SquareBB(randomSquare);
                    isValid = CheckIsValid((COLOR)c, p, randomSquareBB);
                } while(!isValid && tries < 100);
                if(tries >= 100)
                    continue;
                board.m_pieces[c][p] |= randomSquareBB;
                board.m_allpieces |= randomSquareBB;
            }

            } //p
        } //c

    } while(board.IsCheckAnyColor() || !board.GetPieces(WHITE, KING) || !board.GetPieces(BLACK, KING));

    int random = rng.Random(0, 1);
    board.m_activePlayer = random ? WHITE : BLACK;

    board.InitStateAndHistory();

    return GetSimplifiedFen(board);
}

//Fen with only the piece positions (without side-to-move, castling rights...)
std::string Fen::GetSimplifiedFen(Board& board) {
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