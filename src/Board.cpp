#include "Board.h"

#include "Attacks.h"
#include "Uci.h"
using namespace Attacks;
#include "BitboardUtils.h"
using namespace BitboardUtils;
#include "Hash.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "NNUE.h"

#include <iostream>
#include <sstream>
#include <string>

const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const char PIECE_NOTATION[2][8] = { {' ', 'P', 'N', 'B', 'R', 'Q', 'K', '-',},
                                    {' ', 'p', 'n', 'b', 'r', 'q', 'k', '-'} }; //[COLOR][PIECE]

/*
------------
Useful sites
------------
Basics:
    http://pages.cs.wisc.edu/~psilord/blog/data/chess-pages/
Move generation:
    https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/
    https://www.chessprogramming.org/Efficient_Generation_of_Sliding_Piece_Attacks
*/

Board::Board() {
    Init();
}

void Board::Init() {
    SetFen(STARTFEN);
}

u64 Board::Perft(int depth) {
    u64 nodes = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    if(depth == 0) return 1;
    if(depth == 1) return moves.size();

    for(auto &move : moves) {
        
        //Integrity check: before
        D( Board boardBefore = *this; );

        MakeMove(move);
        nodes += Perft(depth - 1);
        TakeMove(move);

        //Integrity check: after
        D( Board boardAfter = *this; );
        D( assert(boardBefore == boardAfter); );
    }

    return nodes;
}

void Board::Divide(int depth) {
    u64 nodesTotal = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    for(auto &move : moves) {
        MakeMove(move);
        u64 nodes = Perft(depth-1);
        P( move.Notation() << " \t" << nodes );
        nodesTotal += nodes;
        TakeMove(move);
    }

    P("nodes: " << nodesTotal);
}

void Board::Print(bool bits) const {
    if(bits) {
        PrintBits( m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES] );
        return;
    }

    std::string squareMap[64];
    for (int i = 0; i < 64; ++i) {
        squareMap[i] = PIECE_NOTATION[0][NO_PIECE];
    }

    for(COLOR color : {WHITE, BLACK}) {
        for(PIECE_TYPE piece = PAWN; piece <= KING; ++piece) {
            Bitboard thePieces = m_pieces[color][piece];
            while(thePieces) {
                int index = ResetLsb(thePieces);
                squareMap[index] = '\0';
                squareMap[index] += (color == WHITE) ? "\033[1;97m" : "\033[1;31m"; //white or red
                squareMap[index] += PIECE_NOTATION[color][piece];
                squareMap[index] += "\033[0m";
            }
        }
    }

    std::cout << "--------------------------------" << std::endl;
    int square = A8;
    const int nextRank = 8;
    while(square >= 0) {
        std::cout << " " << squareMap[square] << " |";
        if(File(square) == FILEH) {
            square -= nextRank * 2;
            std::cout << std::endl << "--------------------------------" << std::endl;
        }
        square++;
    }
}

void Board::ShowHashMoves() {
    MoveGenerator gen;
    MoveList moves = gen.GenerateMoves(*this);

    for(auto move : moves)  {
        MakeMove(move);
        TTEntry* ttEntry = Hash::tt.ProbeEntry(ZKey(), 0);
        if(ttEntry) {
            P(move.Notation() << " " << ttEntry->type << "\t" << ttEntry->score);
        }
        TakeMove(move);
    }
}

void Board::ShowHistory() {
    for(uint i = m_initialPly + 1; i <= m_ply; i++) {
        std::cout << i << ". " << m_history[i].move.Notation() << " ";
    }
}

void Board::ShowMoves() {
    MoveGenerator moveGenerator;
    MoveList moves = moveGenerator.GenerateMoves(*this);
    for(auto move : moves ) {
        move.Print();
    }
    P("size: " << moves.size());
}

//Static Exchange Evaluator
int Board::SEE(Move move) {
    //Retrieve info
    COLOR color = ActivePlayer();
    COLOR enemyColor = InactivePlayer();
    MoveData moveData = move.Data();

    //List of scores to be filled
    const int MAX_CAPTURES = 24;
    int scores[MAX_CAPTURES];
    int numCapture = 0;

    //Direct attackers to the square
    Bitboard attackers = XRayAttackersTo(color, moveData.toSq) | XRayAttackersTo(enemyColor, moveData.toSq);

    //Initial capture
    attackers ^= SquareBB(moveData.fromSq);
    scores[numCapture] = SEE_MATERIAL_VALUES[moveData.capturedType];
    int pieceValue = SEE_MATERIAL_VALUES[moveData.pieceType]; //what is actually on the square

    // switch the active player
    color = (COLOR)!color;
    enemyColor = (COLOR)!enemyColor;

    //the main loop, where all the captures are simulated
    while(attackers) {
        Bitboard activeAttackers = attackers & Piece(color, ALL_PIECES);
        if(!activeAttackers) break;

        //get the Least Valuable Attacker
        PIECE_TYPE pieceType;
        Bitboard LVA = LeastValuableAttacker(activeAttackers, color, pieceType);
        attackers ^= LVA;

        //if king... do smth

        //fill an array of scores / current-piece-on-square
        numCapture++;
        assert(numCapture <= MAX_CAPTURES);
        scores[numCapture] = pieceValue - scores[numCapture-1];
        pieceValue = SEE_MATERIAL_VALUES[pieceType];

        // switch the active player
        color = (COLOR)!color;
        enemyColor = (COLOR)!enemyColor;
    }

    // add hidden pieces
    //another options is to check for bishops, rooks and queen in the moving direction?? (don't do for kings and knights)

    //retrieve best score
    while(numCapture > 0) {
        if(scores[numCapture] > -scores[numCapture-1]) { //scores[numCapture-1] > -scores[numCapture]
            scores[numCapture-1] = -scores[numCapture];
        }
        numCapture--;
    }

    return scores[0];
}

//Attackers
Bitboard Board::AttackersTo(COLOR color, int square) const {
    Bitboard attackers = ZERO;

    COLOR enemyColor = (COLOR)!color;

    //Non-sliding
    attackers |= AttacksPawns(color, square) & GetPieces(enemyColor, PAWN);
    attackers |= AttacksKnights(square) & GetPieces(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & GetPieces(enemyColor, KING);

    //Eliminate enemy king for sliding calculation
    Bitboard blockers = m_allpieces ^ GetPieces(color, KING);

    //Bishop
    Bitboard potentialAttackers = AttacksSliding(BISHOP, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, BISHOP) | GetPieces(enemyColor, QUEEN) );

    //Rook
    potentialAttackers = AttacksSliding(ROOK, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, ROOK) | GetPieces(enemyColor, QUEEN) );

    return attackers;
}

Bitboard Board::AttackersTo(COLOR color, int square, Bitboard blockers) const {
    Bitboard attackers = ZERO;
    COLOR enemyColor = (COLOR)!color;

    //Non-sliding
    attackers |= AttacksPawns(color, square) & GetPieces(enemyColor, PAWN);
    attackers |= AttacksKnights(square) & GetPieces(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & GetPieces(enemyColor, KING);

    //Bishop
    Bitboard potentialAttackers = AttacksSliding(BISHOP, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, BISHOP) | GetPieces(enemyColor, QUEEN) );

    //Rook
    potentialAttackers = AttacksSliding(ROOK, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, ROOK) | GetPieces(enemyColor, QUEEN) );

    return attackers;
}

PIECE_TYPE Board::GetPieceAtSquare(COLOR color, int square) const {
    Bitboard bit = SquareBB(square);

    PIECE_TYPE piece = NO_PIECE;

    if(      bit & m_pieces[color][PAWN] )   piece = PAWN;
    else if( bit & m_pieces[color][KNIGHT] ) piece = KNIGHT;
    else if( bit & m_pieces[color][BISHOP] ) piece = BISHOP;
    else if( bit & m_pieces[color][ROOK] )   piece = ROOK;
    else if( bit & m_pieces[color][QUEEN] )  piece = QUEEN;
    else if( bit & m_pieces[color][KING] )   piece = KING;

    return piece;
}

bool Board::IsAttacked(COLOR color, int square) const {
    COLOR enemyColor = (COLOR)!color;

    if( AttacksPawns(color, square) & Piece(enemyColor, PAWN) ) return true;
    if( AttacksKnights(square) & Piece(enemyColor, KNIGHT) ) return true;
    if( AttacksKing(square) & Piece(enemyColor, KING) ) return true;

    Bitboard blockers = m_allpieces;
    Bitboard diagonalPieces = Piece(enemyColor, BISHOP) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(BISHOP, square, blockers) & diagonalPieces ) return true;
    Bitboard straightPieces = Piece(enemyColor, ROOK) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(ROOK, square, blockers) & straightPieces ) return true;

    return false;
}

bool Board::IsCheck() {
    COLOR color = ActivePlayer();
    if(!m_checkCalculated) {
        UpdateKingAttackers(color);
        m_checkCalculated = true;
    }
    return m_kingAttackers[color];
}

bool Board::IsCheckAnyColor() {
    //Active color
    m_checkCalculated = false;
    bool check_active = IsCheck();

    //Inactive color
    m_checkCalculated = false;
    m_activePlayer = (COLOR)!m_activePlayer;
    bool check_inactive = IsCheck();
    m_activePlayer = (COLOR)!m_activePlayer;
    m_checkCalculated = false;

    return check_active || check_inactive;
}

bool Board::IsRepetitionDraw(int searchPly) {
    int rep = 0;

    int imax = std::max(searchPly, (int)m_fiftyrule);
    int ply = m_ply - m_initialPly;

    int i = 4;
    while(i <= imax && i <= ply) {
        if(ZKey() == m_history[m_ply - i].zkey) rep++;
        if(rep == 1 && searchPly > i) return true;
        if(rep == 2) return true;
        i += 2;
    }
    return false;
}

void Board::Mirror() {
    m_activePlayer = (COLOR)!m_activePlayer;
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        m_pieces[WHITE][pieceType] = BitboardUtils::Mirror(m_pieces[WHITE][pieceType]);
        m_pieces[BLACK][pieceType] = BitboardUtils::Mirror(m_pieces[BLACK][pieceType]);
        std::swap(m_pieces[WHITE][pieceType], m_pieces[BLACK][pieceType]);
    }
    if(m_enPassantSquare)
        m_enPassantSquare = BitboardUtils::Mirror(m_enPassantSquare);
    if(m_castlingRights) {
        u8 mirroredCastlingRights = 0;
        if(m_castlingRights & CASTLING_K) mirroredCastlingRights ^= CASTLING_k;
        if(m_castlingRights & CASTLING_Q) mirroredCastlingRights ^= CASTLING_q;
        if(m_castlingRights & CASTLING_k) mirroredCastlingRights ^= CASTLING_K;
        if(m_castlingRights & CASTLING_q) mirroredCastlingRights ^= CASTLING_Q;
        m_castlingRights = mirroredCastlingRights;
    }
    m_zobristKey.SetKey(*this);
    m_pawnKey.SetPawnKey(*this);
    UpdateBitboards();
}

int Board::SquareToIndex(std::string square) const {
    const int asciiToNumber = -96;  //lowercase to numbers

    int file = square[0] + asciiToNumber;
    int rank = atoi(&square[1]);

    int index = (file-1) + (rank-1)*8;
    assert(index >= 0 && index < 64);
    return index;
}

Bitboard Board::XRayAttackersTo(COLOR color, int square) {
    Bitboard attackers = ZERO;

    COLOR enemyColor = (COLOR)!color;

    //Non-sliding
    Bitboard pawnAttackers = AttacksPawns(color, square) & Piece(enemyColor, PAWN);
    attackers |= pawnAttackers;
    attackers |= AttacksKnights(square) & Piece(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & Piece(enemyColor, KING);

    //Eliminate enemy king for sliding calculation
    Bitboard blockers = m_allpieces ^ Piece(color, KING); //will be attacked after check

    //Bishop
    pawnAttackers |= AttacksPawns(enemyColor, square) & Piece(color, PAWN); //add all pawns (TEST!)
    Bitboard diagonalPieces = Piece(enemyColor, BISHOP) | Piece(enemyColor, QUEEN);
    Bitboard diagonalBlockers = blockers ^ (pawnAttackers | diagonalPieces);
    attackers |= AttacksSliding(BISHOP, square, diagonalBlockers) & diagonalPieces;

    //Rook
    Bitboard straightPieces = Piece(enemyColor, ROOK) | Piece(enemyColor, QUEEN);
    Bitboard straightBlockers = blockers ^ straightPieces;
    attackers |= AttacksSliding(ROOK, square, straightBlockers) & straightPieces;

    return attackers;
}

Bitboard Board::LeastValuableAttacker(Bitboard attackers, COLOR color, PIECE_TYPE& pieceType) {
    assert(attackers);
    for(PIECE_TYPE ipiece = PAWN; ipiece <= KING; ++ipiece) {
        Bitboard pieceAttackers = attackers & Piece(color, ipiece);
        if(pieceAttackers) {
            pieceType = ipiece;
            return IsolateLsb(pieceAttackers);
        }
    }
    assert(false);
    return ZERO;
}

//Operators
bool Board::operator==(const Board& rhs) const {
    bool pieceIntegrity = true;
    for(COLOR color : {WHITE, BLACK}) {
        for(PIECE_TYPE piece = PAWN; piece <= KING; ++piece) {
            if( this->GetPieces(color, piece) != rhs.GetPieces(color, piece) )
                pieceIntegrity = false;
        }
    }
    if( this->m_allpieces != rhs.m_allpieces )
                pieceIntegrity = false;
    
    return pieceIntegrity && 
        this->ActivePlayer() == rhs.ActivePlayer() &&
        this->CastlingRights() == rhs.CastlingRights() &&
        this->EnPassantSquare() == rhs.EnPassantSquare() &&
        this->MoveNumber() == rhs.MoveNumber() &&
        this->Ply() == rhs.Ply() &&
        this->ZKey() == rhs.ZKey();
}

//Private
void Board::ClearBits() {
    for(COLOR color : {WHITE, BLACK}) {
        m_kingAttackers[color] = 0;

        for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
            m_pieces[color][pieceType] = 0;
        }
    }

    for(int i = 0; i < MAX_PLY; ++i) {
        m_history[i].Clear();
    }

    m_enPassantSquare = ZERO;
    m_castlingRights = 0;

    m_activePlayer = WHITE;
    m_moveNumber = 1;
    m_ply = 0;
    m_initialPly = 0;
    m_fiftyrule = 0;

    UpdateBitboards();
}

void Board::UpdateBitboards() {
    m_pieces[WHITE][ALL_PIECES] =
        m_pieces[WHITE][PAWN] | 
        m_pieces[WHITE][KNIGHT] | 
        m_pieces[WHITE][BISHOP] | 
        m_pieces[WHITE][ROOK] | 
        m_pieces[WHITE][QUEEN] | 
        m_pieces[WHITE][KING];
    m_pieces[BLACK][ALL_PIECES] =
        m_pieces[BLACK][PAWN] | 
        m_pieces[BLACK][KNIGHT] | 
        m_pieces[BLACK][BISHOP] | 
        m_pieces[BLACK][ROOK] | 
        m_pieces[BLACK][QUEEN] | 
        m_pieces[BLACK][KING];
    m_allpieces = m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES];
}

void Board::UpdateKingAttackers(COLOR color) {
    Bitboard theKing = GetPieces(color, KING);
    int kingSquare = BitscanForward(theKing);

    m_kingAttackers[color] = AttackersTo(color, kingSquare);
}

void Board::InitStateAndHistory() {
    assert(m_ply >= 0);

    UpdateBitboards();

    m_history[m_ply].fiftyrule = m_fiftyrule;
    m_history[m_ply].castling = m_castlingRights;
    m_history[m_ply].enpassant = m_enPassantSquare;
    m_zobristKey.SetKey(*this);
    m_pawnKey.SetPawnKey(*this);
    m_history[m_ply].zkey = ZKey();

    m_checkCalculated = false;

    if(!UCI_CLASSICAL_EVAL) {
        nnue.SetPieces(WHITE, m_pieces[WHITE][NO_PIECE]);
        nnue.SetPieces(BLACK, m_pieces[BLACK][NO_PIECE]);
        nnue.Inputs_FullUpdate();
    }
}

bool Board::CheckIntegrity() const {
    return PopCount( Piece(WHITE, KING) ) == 1
             && PopCount( Piece(BLACK, KING) ) == 1
             && PopCount( Piece(WHITE, PAWN) ) <= 8
             && PopCount( Piece(BLACK, PAWN) ) <= 8
             && PopCount( EnPassantSquare() ) <= 1
             && Ply() <= MAX_PLY
             && ActivePlayer() != NO_COLOR
        ;
}
