#include "Board.h"

#include "Attacks.h"
using namespace Attacks;
#include "BitboardUtils.h"
using namespace BitboardUtils;
#include "Evaluation.h"
#include "Move.h"
#include "MoveGenerator.h"

#include <iostream>
#include <sstream>
#include <string>

const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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
    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();
    Init();
}

void Board::Init() {
    SetFen(STARTFEN);
}

void Board::Divide(int depth) {
    U64 nodesTotal = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    for(auto &move : moves) {
        MakeMove(move);
        U64 nodes = Perft(depth-1);
        P( move.Notation() << " \t" << nodes );
        nodesTotal += nodes;
        TakeMove(move);
    }

    P("nodes: " << nodesTotal);
}

void Board::Mirror() {
    m_activePlayer = (COLORS)!m_activePlayer;
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        m_pieces[WHITE][pieceType] = BitboardUtils::Mirror(m_pieces[WHITE][pieceType]);
        m_pieces[BLACK][pieceType] = BitboardUtils::Mirror(m_pieces[BLACK][pieceType]);
        std::swap(m_pieces[WHITE][pieceType], m_pieces[BLACK][pieceType]);
    }
    if(m_enPassantSquare)
        m_enPassantSquare = BitboardUtils::Mirror(m_enPassantSquare);
    if(m_castlingRights) {
        U8 mirroredCastlingRights = 0;
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

U64 Board::Perft(int depth) {
    U64 nodes = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    if(depth == 0) return 1;
    if(depth == 1) return moves.size();

    for(auto &move : moves) {
        
        //Integrity check: before
        #ifdef DEBUG
        Board boardBefore = *this;
        #endif

        MakeMove(move);
        nodes += Perft(depth - 1);
        TakeMove(move);

        //Integrity check: after
        #ifdef DEBUG
        Board boardAfter = *this;
        assert(boardBefore == boardAfter);
        #endif
    }

    return nodes;
}

void Board::Print(bool bits) const {
    if(bits) {
        PrintBits( m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES] );
        return;
    }

    std::string squareMap[64];
    for (int i = 0; i < 64; ++i) {
        squareMap[i] = PIECE_LETTERS[NO_PIECE];
    }

    for(COLORS color : {WHITE, BLACK}) {
        for(PIECE_TYPE piece = PAWN; piece <= KING; ++piece) {
            Bitboard thePieces = m_pieces[color][piece];
            while(thePieces) {
                int index = ResetLsb(thePieces);
                squareMap[index] = '\0';
                squareMap[index] += (color == WHITE) ? "\033[1;97m" : "\033[1;31m";
                squareMap[index] += PIECE_LETTERS[ PieceTypeToPieces(piece, color) ];
                squareMap[index] += "\033[0m";
            }
        }
    }

    std::cout << "--------------------------------" << std::endl;
    int i = A8;
    while(i != -8) {
        std::cout << " " << squareMap[i] << " |";
        if(i % 8 == 7) {
            i -= 16;
            std::cout << std::endl << "--------------------------------" << std::endl;
        }
        i++;
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
    P("size: " << moveGenerator.Moves().size());
}

//Attackers
Bitboard Board::AttackersTo(COLORS color, int square) const {
    Bitboard attackers = ZERO;

    COLORS enemyColor = (COLORS)!color;

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

Bitboard Board::AttackersTo(COLORS color, int square, Bitboard blockers) const {
    Bitboard attackers = ZERO;
    COLORS enemyColor = (COLORS)!color;

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

PIECE_TYPE Board::GetPieceAtSquare(COLORS color, int square) const {
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

bool Board::IsAttacked(COLORS color, int square) const {
    COLORS enemyColor = (COLORS)!color;

    if( AttacksPawns(color, square) & Piece(enemyColor, PAWN) ) return true;
    if( AttacksKnights(square) & Piece(enemyColor, KNIGHT) ) return true;
    if( AttacksKing(square) & Piece(enemyColor, KING) ) return true;

    Bitboard blockers = m_allpieces;
    Bitboard slidingPieces = Piece(enemyColor, BISHOP) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(BISHOP, square, blockers) & slidingPieces ) return true;
    Bitboard straightPieces = Piece(enemyColor, ROOK) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(ROOK, square, blockers) & straightPieces ) return true;

    return false;
}

bool Board::IsCheck() {
    COLORS color = ActivePlayer();
    if(!m_checkCalculated) {
        UpdateKingAttackers(color);
        m_checkCalculated = true;
    }
    return m_kingAttackers[color];
}

bool Board::IsRepetitionDraw(int searchPly) {
    int rep = 0;

    int imax = std::max(searchPly, (int)m_fiftyrule);
    if(imax < 4) //FIXME
        return false;

    for(int i = 4; i <= imax; i += 2) {
        // if(m_ply - (uint)i < 0) return false;
        if((m_ply - (uint)i) < m_initialPly) return false; //non-defined history
        // assert(m_ply >= (uint)i);

        if(ZKey() == m_history[m_ply - i].zkey) rep++;
        if(rep == 1 && searchPly > i) return true;
        if(rep == 2) return true;
    }
    return false;
}

int Board::SquareToIndex(std::string square) const {
    const int asciiToNumber = -96;  //lowercase to numbers

    int file = square[0] + asciiToNumber;
    int rank = atoi(&square[1]);

    int index = (file-1) + (rank-1)*8;
    assert(index >= 0 && index < 64);
    return index;
}

Bitboard Board::XRayAttackersTo(COLORS color, int square) {
    Bitboard attackers = ZERO;

    COLORS enemyColor = (COLORS)!color;

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

//Static Exchange Evaluator
int Board::SEE(Move move) {
    
    // retrieve info
    COLORS color = ActivePlayer();
    COLORS enemyColor = InactivePlayer();
    MoveData data = move.Data();

    // list of scores to be filled
    const int MAX_CAPTURES = 32;
    int scores[MAX_CAPTURES];
    int numCapture = 0;

    // direct attackers
    Bitboard attackers = XRayAttackersTo(color, data.toSq) | XRayAttackersTo(enemyColor, data.toSq);

    // handle separately the initial capture
    attackers ^= SquareBB(data.fromSq);

    // simulate the initial capture
    scores[numCapture] = SEE_MATERIAL_VALUES[data.capturedType];
    int pieceValue = SEE_MATERIAL_VALUES[data.pieceType]; //what is actually on the square

    // switch the active player
    color = (COLORS)!color;
    enemyColor = (COLORS)!enemyColor;

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
        scores[numCapture] = pieceValue - scores[numCapture-1];
        pieceValue = SEE_MATERIAL_VALUES[pieceType];

        // switch the active player
        color = (COLORS)!color;
        enemyColor = (COLORS)!enemyColor;
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

Bitboard Board::LeastValuableAttacker(Bitboard attackers, COLORS color, PIECE_TYPE& pieceType) {
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
    for(COLORS color : {WHITE, BLACK}) {
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
    for(COLORS color : {WHITE, BLACK}) {
        m_attackedSquares[color] = 0;
        m_pinnedPieces[color] = 0;
        m_kingAttackers[color] = 0;
        m_kingDangerSquares[color] = 0;

        for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
            m_pieces[color][pieceType] = 0;
        }
    }

    for(int i = 0; i < MAX_PLIES; ++i) {
        m_history[i].Clear();
    }

    for(int i = 0; i < 63; ++i) {
        m_pinnedPushMask[i] = 0;
        m_pinnedCaptureMask[i] = 0;
    }

    m_enPassantSquare = ZERO;
    m_castlingRights = 0;

    m_activePlayer = WHITE;
    m_moveNumber = 1;
    m_ply = 0;
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

void Board::UpdateKingAttackers(COLORS color) {
    Bitboard theKing = GetPieces(color, KING);
    int kingSquare = BitscanForward(theKing);

    m_kingAttackers[color] = AttackersTo(color, kingSquare);
}

bool Board::CheckIntegrity() const {
    return PopCount( Piece(WHITE, KING) ) == 1
             && PopCount( Piece(BLACK, KING) ) == 1
             && PopCount( Piece(WHITE, PAWN) ) <= 8
             && PopCount( Piece(BLACK, PAWN) ) <= 8
             && PopCount( EnPassantSquare() ) <= 1
             && Ply() <= MAX_PLIES
             && ActivePlayer() != NO_COLOR
        ;
}
